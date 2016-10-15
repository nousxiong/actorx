//
// buffer_pool.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/detail/buffer.hpp"

#include "actorx/asev/all.hpp"

#include <chrono>
#include <array>
#include <atomic>
#include <memory>


namespace actx
{
namespace detail
{
class buffer_pool
{
  buffer_pool(buffer_pool const&) = delete;
  buffer_pool(buffer_pool&&) = delete;
  buffer_pool& operator=(buffer_pool const&) = delete;
  buffer_pool& operator=(buffer_pool&&) = delete;

  using uid_t = unsigned int;

  static size_t constexpr buffer_pool_cnt_ = 26;

#ifdef ASEV_SYSTEM_CLOCK
  using eclipse_clock_t = std::chrono::system_clock;
#else
  using eclipse_clock_t = std::chrono::steady_clock;
#endif // ASEV_SYSTEM_CLOCK

  // Thread local pool.
  struct thread_local_pool : public cque::node_base
  {
    std::unique_ptr<cque::pool_base> pool_;
  };
  // Capacity pool array.
  struct capacity_pool_array
  {
    size_t size_;
    thread_local_pool* arr_[buffer_pool_cnt_];
  };
  // Thread local pool array.
  struct thread_local_pool_array
  {
    size_t size_;
    capacity_pool_array arr_[ASEV_MAX_EV_SERVICE];
  };
  using local_pool_queue_t = cque::mpsc_queue<thread_local_pool, eclipse_clock_t>;

public:
  explicit buffer_pool(uid_t uid, size_t max_cached_buffer_size = 1 << 20)
    : uid_(uid)
    , max_cached_buffer_size_(max_cached_buffer_size)
  {
    // Create capacity array.
    for (size_t i=0, j=7; i<cap_arr_.size() - 1; ++i, ++j)
    {
      cap_arr_[i] = 1 << j;
    }
    cap_arr_[cap_arr_.size() - 1] = std::numeric_limits<size_t>::max();
  }

  ~buffer_pool()
  {
    while (true)
    {
      auto pool = local_pool_queue_.pop_unique();
      if (!pool)
      {
        break;
      }
    }
  }

public:
  buffer_ptr get(size_t size) noexcept
  {
    if (size > max_cached_buffer_size_)
    {
      return std::move(buffer_ptr(new buffer(size)));
    }

    ACTX_EXPECTS(uid_ < ASEV_MAX_EV_SERVICE);
    using buffer_pool_t = cque::mpsc_pool<buffer, buffer_make>;

    // Non-trivial object with thread_local storage hasn't been implemented in gcc yet.
    static thread_local thread_local_pool_array pool_array{0};
    if (pool_array.size_ == 0)
    {
      for (auto& cap_pool_arr : pool_array.arr_)
      {
        cap_pool_arr.size_ = 0;
        for (auto& ptr : cap_pool_arr.arr_)
        {
          ptr = nullptr;
        }
      }
    }

    auto& cap_pool_arr = pool_array.arr_[uid_];
    if (cap_pool_arr.size_ == 0)
    {
      ++pool_array.size_;
    }

    auto pool_index = get_pool_index(size);
    auto& curr_pool = cap_pool_arr.arr_[pool_index];
    if (curr_pool == nullptr)
    {
      auto ptr = new thread_local_pool;
      ptr->pool_.reset(new buffer_pool_t(buffer_make(cap_arr_[pool_index])));
      local_pool_queue_.push(ptr);
      curr_pool = ptr;
      ++cap_pool_arr.size_;
    }

    auto pool = gsl::narrow_cast<buffer_pool_t*>(curr_pool->pool_.get());
    ACTX_ASSERTS(!!pool);

    return std::move(buffer_ptr(pool->get()));
  }

private:
  size_t get_pool_index(size_t capacity) const noexcept
  {
    for (size_t i=0; i<cap_arr_.size(); ++i)
    {
      if (capacity <= cap_arr_[i])
      {
        return i;
      }
    }
    return buffer_pool_cnt_;
  }

private:
  uid_t uid_;
  size_t max_cached_buffer_size_;
  std::array<size_t, buffer_pool_cnt_> cap_arr_;
  std::array<thread_local_pool_array, buffer_pool_cnt_> cpool_array_;

  // Local pool queue.
  local_pool_queue_t local_pool_queue_;
};
} // namespace detail
} // namespace actx
