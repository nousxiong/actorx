//
// basic_strand.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"


namespace asev
{
namespace detail
{
//! Seq queue for async events.
template <typename EvService>
class basic_strand
{
public:
  basic_strand()
    : evs_(nullptr)
    , index_(0)
  {
  }

  explicit basic_strand(EvService& evs) noexcept
    : evs_(&evs)
    , index_(evs_->select_strand_index())
  {
  }

public:
  operator bool() const
  {
    return evs_ != nullptr;
  }

  bool operator!() const
  {
    return evs_ == nullptr;
  }

  //! Post a functor to strand.
  template <typename F>
  void post(F&& f)
  {
    ACTX_ASSERTS(evs_ != nullptr);
    evs_->pri_post(index_, typename EvService::post_handler_t(f));
  }

  //! Spawn an coroutine with strand.
  template <typename F>
  void spawn(F&& f, coctx::stack_size ssize = coctx::make_stacksize())
  {
    ACTX_ASSERTS(evs_ != nullptr);
    evs_->pri_spawn(index_, typename EvService::coro_handler_t(f), ssize);
  }

  //! Add an user defined event into strand to run.
  /**
   * @param ev User defined event.
   */
  void async(gsl::owner<event_base*> ev)
  {
    ACTX_ASSERTS(evs_ != nullptr);
    evs_->pri_async(index_, ev);
  }

  //! Get ev_serivce.
  inline EvService& get_ev_service() noexcept
  {
    ACTX_ASSERTS(evs_ != nullptr);
    return *evs_;
  }

private:
  EvService* evs_;
  size_t index_;
};
}
}
