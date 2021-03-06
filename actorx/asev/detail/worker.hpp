//
// worker.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"
#include "actorx/asev/detail/basic_thrctx.hpp"

#include <chrono>
#include <vector>


namespace asev
{
//! Work level.
enum work_level
{
  prior,
  minor
};

namespace detail
{

//! Worker for handling events.
class worker
{
#ifdef ASEV_SYSTEM_CLOCK
  using eclipse_clock_t = std::chrono::system_clock;
#else
  using eclipse_clock_t = std::chrono::steady_clock;
#endif // ASEV_SYSTEM_CLOCK

public:
  worker(size_t worker_num, size_t index) noexcept
    : index_(index)
    , sworks_(0)
    , pworks_(0)
    , mworks_(0)
  {
  }

  ~worker()
  {
    while (true)
    {
      auto ev = que_.pop_unique<cque::pool_delete<event_base>>();
      if (!ev)
      {
        break;
      }
    }
  }

public:
  size_t get_index() const noexcept
  {
    return index_;
  }

  size_t fetch_sworks() noexcept
  {
    return sworks_.exchange(0, std::memory_order_acquire);
  }

  void push_event(gsl::owner<event_base*> ev) noexcept
  {
    que_.push(ev);
  }

  template <typename EvService>
  size_t work(basic_thrctx<EvService>& thrctx, work_level wlv) noexcept
  {
    size_t works = 0;
    // Event loop.
    while (true)
    {
      auto ev = que_.pop();
      if (ev == nullptr)
      {
        // @todo improve loop strategy.
        break;
      }

      bool is_auto = true;
      try
      {
        ++works;
        is_auto = ev->handle(thrctx);
      }
      catch (...)
      {
        ACTX_ENSURES(false);
      }

      if (is_auto)
      {
        ev->release();
      }
    }

    // Update works.
    switch (wlv)
    {
    case work_level::prior: pworks_ += works; break;
    case work_level::minor:
    {
      mworks_ += works;
      if (works > 0)
      {
        sworks_.fetch_add(works, std::memory_order_release);
      }
    }break;
    }

    return works;
  }

  int64_t get_pworks() const noexcept
  {
    return pworks_;
  }

  int64_t get_mworks() const noexcept
  {
    return mworks_;
  }

private:
  size_t index_;
  cque::mpsc_queue<event_base, eclipse_clock_t> que_;
  // stolen works
  std::atomic_size_t sworks_;

  //! Status.
  int64_t pworks_;
  int64_t mworks_;
};
}
}
