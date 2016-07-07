///
/// mpsc_count.hpp
///

#pragma once

#include "actorx/cque/config.hpp"

#include <chrono>
#include <atomic>
#include <utility>


namespace cque
{
template <typename Clock = std::chrono::high_resolution_clock>
class mpsc_count
{
  using self_t = mpsc_count<Clock>;
  mpsc_count(mpsc_count const&) = delete;
  mpsc_count(mpsc_count&&) = delete;
  mpsc_count& operator=(mpsc_count const&) = delete;
  mpsc_count& operator=(mpsc_count&&) = delete;

public:
  using eclipse_clock_t = Clock;
  using duration_t = typename eclipse_clock_t::duration;

public:
  mpsc_count()
    : count_(0)
    , blocked_(false)
  {
  }

public:
  /// Reset count to zero, return count before reset.
  /**
   * @return Count before reset
   */
  size_t reset() noexcept
  {
    return pri_reset();
  }

  /// Increase count for one.
  void incr() noexcept
  {
    pri_incr();
  }

  /// Increase count for one. if blocking invoke given functor.
  /**
   * @param f A functor invoke on blocking.
   */
  template <typename F>
  void incr(F&& f)
  {
    incr();
    if (blocked())
    {
      f(*this);
    }
  }

  /// Synchronized increase for one, if blocking, wake up it.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waking up.
   */
  template <class Mutex, class CondVar>
  void synchronized_incr(Mutex& mtx, CondVar& cv)
  {
    incr(
      [&](self_t& seq)
      {
        std::unique_lock<Mutex> guard{mtx};
        cv.notify_one();
      });
  }

  /// Synchronized reset count to zero, return count before reset., if is already zero,
  /// Block current thread until any incr coming.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @return Count before reset
   */
  template <class Mutex, class CondVar>
  size_t synchronized_reset(Mutex& mtx, CondVar& cv)
  {
    return pri_synchronized_reset(mtx, cv);
  }

  /// Synchronized reset count to zero, return count before reset., if is already zero,
  /// block current thread for given time until incr coming or timed out.
  /**
   * @param mtx Mutex for lock guard.
   * @param cv Condtion var for waiting.
   * @param timeout Given timeout duration
   * @return Count before reset and eclipse time, if timed out count will be 0
   */
  template <class Mutex, class CondVar, typename TimeDuration>
  std::pair<size_t, duration_t> synchronized_reset(Mutex& mtx, CondVar& cv, TimeDuration const& timeout)
  {
    ACTORX_ASSERTS(!blocked());
    if (timeout <= TimeDuration::zero())
    {
      auto c = reset();
      return std::make_pair(c, TimeDuration::zero());
    }

    if (timeout >= (TimeDuration::max)())
    {
      auto nw = eclipse_clock_t::now();
      auto c = pri_synchronized_reset(mtx, cv);
      return std::make_pair(c, eclipse_clock_t::now() - nw);
    }

    auto c = reset();
    if (c != 0)
    {
      return std::make_pair(c, TimeDuration::zero());
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });

    c = reset();
    if (c != 0)
    {
      return std::make_pair(c, TimeDuration::zero());
    }

    auto const timeout_dur =
      std::chrono::duration_cast<duration_t>(timeout);
    auto curr_timeout = timeout_dur;
    auto final_eclipse = TimeDuration::zero();
    while (c == 0)
    {
      auto bt = eclipse_clock_t::now();
      auto status = cv.wait_for(guard, curr_timeout);
      if (status == std::cv_status::timeout)
      {
        c = reset();
        final_eclipse = timeout_dur;
        break;
      }
      else
      {
        auto eclipse = eclipse_clock_t::now() - bt;
        c = reset();
        final_eclipse += eclipse;
        if (eclipse >= curr_timeout)
        {
          break;
        }

        curr_timeout -= eclipse;
      }
    }

    return std::make_pair(c, final_eclipse);
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

  size_t pri_reset() noexcept
  {
    return count_.exchange(0, std::memory_order_acquire);
  }

  void pri_incr() noexcept
  {
    auto stale_count = count_.load(std::memory_order_relaxed);
    size_t next_count = 0;
    do
    {
      next_count = stale_count + 1;
    }while (!count_.compare_exchange_weak(stale_count, next_count, std::memory_order_release));
  }

  template <class Mutex, class CondVar>
  size_t pri_synchronized_reset(Mutex& mtx, CondVar& cv)
  {
    ACTORX_ASSERTS(!blocked());
    auto c = reset();
    if (c != 0)
    {
      return c;
    }

    std::unique_lock<Mutex> guard{mtx};
    block();
    auto _ = gsl::finally([this]() {unblock(); });
    c = reset();
    while (c == 0)
    {
      cv.wait(guard);
      c = reset();
    }

    return c;
  }

private:
  /// Ensure occupy entire cache(s) line.
  char pad_[CQUE_CACHE_LINE_SIZE];
  CQUE_CACHE_ALIGNED_VAR(std::atomic_size_t, count_);
  CQUE_CACHE_ALIGNED_VAR(std::atomic_bool, blocked_);
};
}

