///
/// mpsc_queue.hpp
///

#pragma once

#include "actorx/cque/config.hpp"
#include "actorx/cque/node_base.hpp"
#include "actorx/cque/variant_ptr.hpp"

#include <chrono>
#include <mutex>
#include <atomic>
#include <limits>
#include <condition_variable> // std::cv_status
#include <memory>


namespace cque
{
/// Intrusive multi-producers one consumer queue.
/**
 * @par Wait-free.
 */
template <typename T, typename Clock = std::chrono::high_resolution_clock>
class mpsc_queue
{
  static_assert(std::is_base_of<node_base, T>::value, "T must inhert from cque::node_base");
  using node_t = node_base;
  using self_t = mpsc_queue<T, Clock>;
  using eclipse_clock_t = Clock;
  using duration_t = typename eclipse_clock_t::duration;

  mpsc_queue(mpsc_queue const&) = delete;
  mpsc_queue(mpsc_queue&&) = delete;
  mpsc_queue& operator=(mpsc_queue const&) = delete;
  mpsc_queue& operator=(mpsc_queue&&) = delete;

public:
  mpsc_queue() noexcept
    : head_(nullptr)
    , blocked_(false)
    , cache_(nullptr)
  {
  }

  ~mpsc_queue() noexcept
  {
    while (true)
    {
      auto e = pri_pop();
      if (e != nullptr)
      {
        e->destory();
      }
      else
      {
        break;
      }
    }
  }

public:
  /// Pop an element from tail.
  /**
   * @note Only call in single-consumer thread.
   */
  gsl::owner<T*> pop() noexcept
  {
    auto e = pri_pop();
    if (e != nullptr)
    {
      Expects(!e->shared());
      e->free_unique();
    }
    return e;
  }

  /// Push an element at head.
  void push(gsl::owner<T*> e) noexcept
  {
    if (e == nullptr)
    {
      return;
    }

    Expects(!e->shared());
    Expects(!e->unique());
    pri_push(e);
  }

  /// Push an element at head, if queue blocking invoke given functor.
  /**
   * @param f A functor invoke on blocking.
   */
  template <typename F>
  void push(gsl::owner<T*> e, F&& f)
  {
    push(e);
    if (blocked())
    {
      f(*this);
    }
  }

  /// Synchronized push an element at head, if queue blocking, wake up it.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waking up.
   * @param e Element will push.
   */
  template <class Mutex, class CondVar>
  void synchronized_push(Mutex& mtx, CondVar& cv, gsl::owner<T*> e)
  {
    push(e,
      [&](self_t& que)
      {
        std::unique_lock<Mutex> guard{mtx};
        cv.notify_one();
      });
  }

  /// Synchronized pop an element from queue, if queue is empty,
  /// Block current thread until any element pushed.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @return An element
   */
  template <class Mutex, class CondVar>
  gsl::owner<T*> synchronized_pop(Mutex& mtx, CondVar& cv)
  {
    return pri_synchronized_pop(mtx, cv);
  }

  /// Synchronized pop an element from queue, if queue is empty,
  /// block current thread for given time until any element pushed or timed out.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @param timeout Given timeout duration
   * @return An element and eclipse time, if timed out it will be null
   */
  template <class Mutex, class CondVar, typename TimeDuration>
  std::pair<gsl::owner<T*>, duration_t> synchronized_pop(Mutex& mtx, CondVar& cv, TimeDuration const& timeout)
  {
    Expects(!blocked());
    if (timeout <= TimeDuration::zero())
    {
      auto e = pop();
      return std::make_pair(e, TimeDuration::zero());
    }

    if (timeout >= (TimeDuration::max)())
    {
      auto nw = eclipse_clock_t::now();
      auto e = pri_synchronized_pop(mtx, cv);
      return std::make_pair(e, eclipse_clock_t::now() - nw);
    }

    auto e = pop();
    if (e != nullptr)
    {
      return std::make_pair(e, TimeDuration::zero());
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    e = pop();
    if (e != nullptr)
    {
      return std::make_pair(e, TimeDuration::zero());
    }

    auto const timeout_dur =
      std::chrono::duration_cast<duration_t>(timeout);
    auto curr_timeout = timeout_dur;
    auto final_eclipse = TimeDuration::zero();
    while (e == nullptr)
    {
      auto bt = eclipse_clock_t::now();
      auto status = cv.wait_for(guard, curr_timeout);
      if (status == std::cv_status::timeout)
      {
        e = pop();
        final_eclipse = timeout_dur;
        break;
      }
      else
      {
        auto eclipse = eclipse_clock_t::now() - bt;
        e = pop();
        final_eclipse += eclipse;
        if (eclipse >= curr_timeout)
        {
          break;
        }

        curr_timeout -= eclipse;
      }
    }

    return std::make_pair(e, final_eclipse);
  }

  /// Pop an shared_ptr element from tail.
  /**
   * @note Only call in single-consumer thread.
   */
  std::shared_ptr<T> pop_shared() noexcept
  {
    auto e = pri_pop();
    if (e != nullptr)
    {
      Expects(e->shared());
      Expects(!e->unique());
      auto ptr = e->free_shared();
      Ensures(ptr);
      return std::move(std::static_pointer_cast<T>(ptr));
    }
    return std::shared_ptr<T>();
  }

  /// Synchronized pop an shared_ptr element from queue, if queue is empty,
  /// block current thread until any element pushed.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @return An shared_ptr element
   */
  template <class Mutex, class CondVar>
  std::shared_ptr<T> synchronized_pop_shared(Mutex& mtx, CondVar& cv)
  {
    return pri_synchronized_pop_shared(mtx, cv);
  }

  /// Synchronized pop an shared_ptr element from queue, if queue is empty,
  /// block current thread for given time until any element pushed or timed out.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @param timeout Given timeout duration
   * @return An shared_ptr element and eclipse time, if timed out it will be null
   */
  template <class Mutex, class CondVar, typename TimeDuration>
  std::pair<std::shared_ptr<T>, duration_t>
    synchronized_pop_shared(Mutex& mtx, CondVar& cv, TimeDuration const& timeout)
  {
    Expects(!blocked());
    if (timeout <= TimeDuration::zero())
    {
      auto e = pop_shared();
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    if (timeout >= (TimeDuration::max)())
    {
      auto nw = eclipse_clock_t::now();
      auto e = pri_synchronized_pop_shared(mtx, cv);
      return std::make_pair(std::move(e), eclipse_clock_t::now() - nw);
    }

    auto e = pop_shared();
    if (e)
    {
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    e = pop_shared();
    if (e)
    {
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    auto const timeout_dur =
      std::chrono::duration_cast<duration_t>(timeout);
    auto curr_timeout = timeout_dur;
    auto final_eclipse = TimeDuration::zero();
    while (!e)
    {
      auto bt = eclipse_clock_t::now();
      auto status = cv.wait_for(guard, curr_timeout);
      if (status == std::cv_status::timeout)
      {
        e = pop_shared();
        final_eclipse = timeout_dur;
        break;
      }
      else
      {
        auto eclipse = eclipse_clock_t::now() - bt;
        e = pop_shared();
        final_eclipse += eclipse;
        if (eclipse >= curr_timeout)
        {
          break;
        }

        curr_timeout -= eclipse;
      }
    }

    return std::make_pair(std::move(e), final_eclipse);
  }

  /// Push an shared_ptr element at head.
  void push(std::shared_ptr<T> e) noexcept
  {
    if (!e)
    {
      return;
    }

    auto p = e.get();
    Expects(!p->shared());
    Expects(!p->unique());
    p->retain(std::move(e));
    pri_push(p);
  }

  /// Push an shared_ptr element at head, if queue blocking invoke given functor.
  /**
   * @param f A functor invoke on blocking.
   */
  template <typename F>
  void push(std::shared_ptr<T> e, F&& f)
  {
    push(std::move(e));
    if (blocked())
    {
      f(*this);
    }
  }

  /// Synchronized push an shared_ptr element at head, if queue blocking, wake up it.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waking up.
   * @param e Shared_ptr element will push.
   */
  template <class Mutex, class CondVar>
  void synchronized_push(Mutex& mtx, CondVar& cv, std::shared_ptr<T> e)
  {
    push(std::move(e),
      [&](self_t& que)
      {
        std::unique_lock<Mutex> guard{mtx};
        cv.notify_one();
      });
  }

  /// Pop an unique_ptr element from tail.
  /**
   * @note Only call in single-consumer thread.
   */
  template <typename D = std::default_delete<T>>
  std::unique_ptr<T, D> pop_unique(D d = D()) noexcept
  {
    auto e = pri_pop();
    if (e != nullptr)
    {
      Expects(!e->shared());
      e->free_unique();
    }
    return std::move(std::unique_ptr<T, D>(e, d));
  }

  /// Synchronized pop an unique_ptr element from queue, if queue is empty,
  /// block current thread until any element pushed.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @param d A deleter for unique_ptr.
   * @return An unique_ptr element, if timed out it will be null.
   */
  template <class Mutex, class CondVar, typename D = std::default_delete<T>>
  std::unique_ptr<T, D> synchronized_pop_unique(Mutex& mtx, CondVar& cv, D d = D())
  {
    return pri_synchronized_pop_unique(mtx, cv, d);
  }

  /// Synchronized pop an unique_ptr element from queue, if queue is empty,
  /// block current thread for given time until any element pushed or timed out.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @param timeout Given timeout duration.
   * @param d A deleter for unique_ptr.
   * @return An unique_ptr element and eclipse time, if timed out it will be null.
   */
  template <class Mutex, class CondVar, typename TimeDuration, typename D = std::default_delete<T>>
  std::pair<std::unique_ptr<T, D>, duration_t>
    synchronized_pop_unique(Mutex& mtx, CondVar& cv, TimeDuration const& timeout, D d = D())
  {
    Expects(!blocked());
    if (timeout <= TimeDuration::zero())
    {
      auto e = pop_unique(d);
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    if (timeout >= (TimeDuration::max)())
    {
      auto nw = eclipse_clock_t::now();
      auto e = pri_synchronized_pop_unique(mtx, cv, d);
      return std::make_pair(std::move(e), eclipse_clock_t::now() - nw);
    }

    auto e = pop_unique(d);
    if (e)
    {
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    e = pop_unique(d);
    if (e)
    {
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    auto const timeout_dur =
      std::chrono::duration_cast<duration_t>(timeout);
    auto curr_timeout = timeout_dur;
    auto final_eclipse = TimeDuration::zero();
    while (!e)
    {
      auto bt = eclipse_clock_t::now();
      auto status = cv.wait_for(guard, curr_timeout);
      if (status == std::cv_status::timeout)
      {
        e = pop_unique(d);
        final_eclipse = timeout_dur;
        break;
      }
      else
      {
        auto eclipse = eclipse_clock_t::now() - bt;
        e = pop_unique(d);
        final_eclipse += eclipse;
        if (eclipse >= curr_timeout)
        {
          break;
        }

        curr_timeout -= eclipse;
      }
    }

    return std::make_pair(std::move(e), final_eclipse);
  }

  /// Push an unique_ptr element at head.
  template <typename D>
  void push(std::unique_ptr<T, D>&& e) noexcept
  {
    if (!e)
    {
      return;
    }

    auto p = e.release();
    Expects(!p->shared());
    Expects(!p->unique());
    p->retain();
    pri_push(p);
  }

  /// Push an unique_ptr element at head, if queue blocking invoke given functor.
  /**
   * @param f A functor invoke on blocking.
   */
  template <typename D, typename F>
  void push(std::unique_ptr<T, D>&& e, F&& f)
  {
    push(std::move(e));
    if (blocked())
    {
      f(*this);
    }
  }

  /// Synchronized push an unique_ptr element at head, if queue blocking, wake up it.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waking up.
   * @param e Unique_ptr element will push.
   */
  template <class Mutex, class CondVar, typename D>
  void synchronized_push(Mutex& mtx, CondVar& cv, std::unique_ptr<T, D>&& e)
  {
    push(std::move(e),
      [&](self_t& que)
      {
        std::unique_lock<Mutex> guard{mtx};
        cv.notify_one();
      });
  }

  /// Pop a variant_ptr element at tail.
  template <typename D = std::default_delete<T>>
  variant_ptr<T, D> pop_variant(D d = D()) noexcept
  {
    auto e = pri_pop();
    if (e != nullptr)
    {
      if (e->shared())
      {
        Expects(!e->unique());
        auto ptr = e->free_shared();
        Ensures(!!ptr);
        return std::move(variant_ptr<T, D>(std::static_pointer_cast<T>(ptr)));
      }
      else if (e->unique())
      {
        Expects(!e->shared());
        e->free_unique();
        return std::move(variant_ptr<T, D>(std::unique_ptr<T, D>(e)));
      }
      else
      {
        Expects(!e->unique());
        Expects(!e->shared());
        return std::move(variant_ptr<T, D>(e));
      }
    }
    return std::move(variant_ptr<T, D>());
  }

  /// Synchronized pop an variant_ptr element from queue, if queue is empty,
  /// block current thread until any element pushed.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @param d A deleter for variant_ptr.
   * @return An variant_ptr element, if timed out it will be null.
   */
  template <class Mutex, class CondVar, typename D = std::default_delete<T>>
  variant_ptr<T, D> synchronized_pop_variant(Mutex& mtx, CondVar& cv, D d = D())
  {
    return pri_synchronized_pop_variant(mtx, cv, d);
  }

  /// Synchronized pop an variant_ptr element from queue, if queue is empty,
  /// block current thread for given time until any element pushed or timed out.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @param timeout Given timeout duration.
   * @param d A deleter for variant_ptr.
   * @return An variant_ptr element and eclipse time, if timed out it will be null.
   */
  template <class Mutex, class CondVar, typename TimeDuration, typename D = std::default_delete<T>>
  std::pair<variant_ptr<T, D>, duration_t>
    synchronized_pop_variant(Mutex& mtx, CondVar& cv, TimeDuration const& timeout, D d = D())
  {
    Expects(!blocked());
    if (timeout <= TimeDuration::zero())
    {
      auto e = pop_variant(d);
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    if (timeout >= (TimeDuration::max)())
    {
      auto nw = eclipse_clock_t::now();
      auto e = pri_synchronized_pop_variant(mtx, cv, d);
      return std::make_pair(std::move(e), eclipse_clock_t::now() - nw);
    }

    auto e = pop_variant(d);
    if (e)
    {
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    e = pop_variant(d);
    if (e)
    {
      return std::make_pair(std::move(e), TimeDuration::zero());
    }

    auto const timeout_dur =
      std::chrono::duration_cast<duration_t>(timeout);
    auto curr_timeout = timeout_dur;
    auto final_eclipse = TimeDuration::zero();
    while (!e)
    {
      auto bt = eclipse_clock_t::now();
      auto status = cv.wait_for(guard, curr_timeout);
      if (status == std::cv_status::timeout)
      {
        e = pop_variant(d);
        final_eclipse = timeout_dur;
        break;
      }
      else
      {
        auto eclipse = eclipse_clock_t::now() - bt;
        e = pop_variant(d);
        final_eclipse += eclipse;
        if (eclipse >= curr_timeout)
        {
          break;
        }

        curr_timeout -= eclipse;
      }
    }

    return std::make_pair(std::move(e), final_eclipse);
  }

private:
  bool blocked() const noexcept
  {
    return blocked_.load(std::memory_order_acquire);
  }

  void block() noexcept
  {
    blocked_.store(true, std::memory_order_release);
  }

  void unblock() noexcept
  {
    blocked_.store(false, std::memory_order_release);
  }

  gsl::owner<T*> pri_pop() noexcept
  {
    auto e = get_cache();
    if (e == nullptr)
    {
      e = head_.exchange(nullptr, std::memory_order_acquire);
      if (e != nullptr && e->get_next() != nullptr)
      {
        /// reverse
        auto last = e;
        node_t* first = nullptr;
        while (last != nullptr)
        {
          auto tmp = last;
          last = last->get_next();
          tmp->set_next(first);
          first = tmp;
        }
        e = first;
        set_cache(e->fetch_next());
      }
    }

    return gsl::narrow_cast<gsl::owner<T*>>(e);
  }

  /// Push an element at head.
  void pri_push(gsl::owner<T*> e) noexcept
  {
    Expects(e != nullptr);
    auto stale_head = head_.load(std::memory_order_relaxed);
    do
    {
      e->set_next(stale_head);
    }while (!head_.compare_exchange_weak(stale_head, e, std::memory_order_release));
  }

  template <class Mutex, class CondVar>
  gsl::owner<T*> pri_synchronized_pop(Mutex& mtx, CondVar& cv)
  {
    Expects(!blocked());
    auto e = pop();
    if (e != nullptr)
    {
      return e;
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    e = pop();
    while (e == nullptr)
    {
      cv.wait(guard);
      e = pop();
    }

    return e;
  }

  template <class Mutex, class CondVar>
  std::shared_ptr<T> pri_synchronized_pop_shared(Mutex& mtx, CondVar& cv)
  {
    Expects(!blocked());
    auto e = pop_shared();
    if (e)
    {
      return std::move(e);
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    e = pop_shared();
    while (!e)
    {
      cv.wait(guard);
      e = pop_shared();
    }

    return std::move(e);
  }

  template <class Mutex, class CondVar, typename D>
  std::unique_ptr<T, D> pri_synchronized_pop_unique(Mutex& mtx, CondVar& cv, D d)
  {
    Expects(!blocked());
    auto e = pop_unique(d);
    if (e)
    {
      return std::move(e);
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    e = pop_unique(d);
    while (!e)
    {
      cv.wait(guard);
      e = pop_unique(d);
    }

    return std::move(e);
  }

  template <class Mutex, class CondVar, typename D>
  variant_ptr<T, D> pri_synchronized_pop_variant(Mutex& mtx, CondVar& cv, D d)
  {
    Expects(!blocked());
    auto e = pop_variant(d);
    if (e)
    {
      return std::move(e);
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    e = pop_variant(d);
    while (!e)
    {
      cv.wait(guard);
      e = pop_variant(d);
    }

    return std::move(e);
  }

private:
  gsl::owner<node_t*> get_cache() noexcept
  {
    auto n = cache_;
    if (n != nullptr)
    {
      cache_ = n->fetch_next();
    }
    return n;
  }

  void set_cache(gsl::owner<node_t*> n) noexcept
  {
    Expects(cache_ == nullptr);
    cache_ = n;
  }

private:
  /// Ensure occupy entire cache(s) line.
  char pad_[CQUE_CACHE_LINE_SIZE];
  CQUE_CACHE_ALIGNED_VAR(std::atomic<gsl::owner<node_t*>>, head_);
  CQUE_CACHE_ALIGNED_VAR(std::atomic_bool, blocked_);

  /// Only access by owner(pop thread).
  gsl::owner<node_t*> cache_;
};
}
