//
// actor_pool.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/detail/actor_base.hpp"

#include <chrono>
#include <array>
#include <atomic>
#include <memory>


namespace actx
{
namespace detail
{
template <class Actor>
class actor_pool
{
  actor_pool(actor_pool const&) = delete;
  actor_pool(actor_pool&&) = delete;
  actor_pool& operator=(actor_pool const&) = delete;
  actor_pool& operator=(actor_pool&&) = delete;

  using uid_t = unsigned int;

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
  // Thread local pool array.
  struct thread_local_pool_array
  {
    size_t size_;
    thread_local_pool* arr_[ASEV_MAX_EV_SERVICE];
  };
  using local_pool_queue_t = cque::mpsc_queue<thread_local_pool, eclipse_clock_t>;

public:
  actor_pool(uid_t uid, buffer_pool* pool, atom_t axid, int64_t timestamp)
    : uid_(uid)
    , pool_(pool)
    , axid_(axid)
    , timestamp_(timestamp)
  {
  }

  ~actor_pool()
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
  Actor* get() noexcept
  {
    ACTX_EXPECTS(uid_ < ASEV_MAX_EV_SERVICE);
    using actor_pool_t = cque::mpsc_pool<Actor, actor_make<Actor>>;

    // Non-trivial object with thread_local storage hasn't been implemented in gcc yet.
    static thread_local thread_local_pool_array pool_array{0};
    if (pool_array.size_ == 0)
    {
      for (auto& ptr : pool_array.arr_)
      {
        ptr = nullptr;
      }
    }

    auto& curr_pool = pool_array.arr_[uid_];
    if (curr_pool == nullptr)
    {
      auto ptr = new thread_local_pool;
      ptr->pool_.reset(new actor_pool_t(actor_make<Actor>(pool_, axid_, timestamp_)));
      local_pool_queue_.push(ptr);
      curr_pool = ptr;
      ++pool_array.size_;
    }

    auto pool = gsl::narrow_cast<actor_pool_t*>(curr_pool->pool_.get());
    ACTX_ASSERTS(pool != nullptr);

    return cque::get<Actor>(*pool);
  }

private:
  uid_t uid_;
  buffer_pool* pool_;
  atom_t const axid_;
  int64_t const timestamp_;

  // Local pool queue.
  local_pool_queue_t local_pool_queue_;
};
} // namespace detail
} // namespace actx
