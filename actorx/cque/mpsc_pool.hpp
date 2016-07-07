///
/// mpsc_pool.hpp
///

#pragma once

#include "actorx/cque/config.hpp"
#include "actorx/cque/node_base.hpp"
#include "actorx/cque/pool_base.hpp"

#include <atomic>
#include <limits>


namespace cque
{
/// Multi-producers one consumer node pool.
template <typename T, typename PoolMake = pool_make<T>>
class mpsc_pool
  : public pool_base
{
  static_assert(std::is_base_of<node_base, T>::value, "T must inhert from cque::node_base");
  using node_t = node_base;

  mpsc_pool(mpsc_pool const&) = delete;
  mpsc_pool(mpsc_pool&&) = delete;
  mpsc_pool& operator=(mpsc_pool const&) = delete;
  mpsc_pool& operator=(mpsc_pool&&) = delete;

public:
  explicit mpsc_pool(PoolMake pmk = PoolMake()) noexcept
    : pmk_(std::move(pmk))
    , head_(nullptr)
    , cache_(nullptr)
  {
  }

  ~mpsc_pool() noexcept
  {
    while (true)
    {
      auto n = pri_get();
      if (!n)
      {
        break;
      }

      n->on_get(nullptr);
      n->release();
    }
  }

public:
  /// Warn: only call in single-consumer thread.
  gsl::owner<node_t*> get() noexcept override
  {
    auto n = pri_get();
    if (n == nullptr)
    {
      n = pmk_();
    }

    if (n != nullptr)
    {
      n->on_get(this);
    }
    return n;
  }

  bool free(gsl::owner<node_t*> n) noexcept override
  {
    if (n == nullptr)
    {
      return true;
    }

    n->on_free();

    auto stale_head = head_.load(std::memory_order_relaxed);
    do
    {
      n->set_next(stale_head);
    }while (!head_.compare_exchange_weak(stale_head, n, std::memory_order_release));
    return true;
  }

private:
  node_t* pri_get() noexcept
  {
    auto n = get_cache();
    if (n == nullptr)
    {
      n = head_.exchange(nullptr, std::memory_order_acquire);
      if (n != nullptr && n->get_next() != nullptr)
      {
        set_cache(n->get_next());
      }
    }
    return n;
  }

  node_t* get_cache() noexcept
  {
    auto n = cache_;
    if (n != nullptr)
    {
      cache_ = n->get_next();
      n->set_next(nullptr);
    }
    return n;
  }

  void set_cache(node_t* n) noexcept
  {
    ACTORX_ASSERTS(cache_ == nullptr);
    cache_ = n;
  }

private:
  PoolMake pmk_;
  std::atomic<node_t*> head_;
  node_t* cache_;
};
}
