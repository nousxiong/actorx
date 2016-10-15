//
// spawn_event.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"
#include "actorx/asev/detail/basic_corctx.hpp"
#include "actorx/asev/detail/basic_thrctx.hpp"

#include <functional>


namespace asev
{
namespace detail
{
//! Spawn event, for spawn an corctx.
template <typename EvService>
class spawn_event : public asev::event_base
{
  using strand_t = basic_strand<EvService>;
  using corctx_t = basic_corctx<EvService>;
  using corctx_make_t = corctx_make<EvService>;
  using handler_t = std::function<void (corctx_t&)>;

public:
  //! Set handler to run.
  void set_handler(handler_t hdr)
  {
    hdr_ = std::move(hdr);
  }

  void set_stack_size(coctx::stack_size ssize)
  {
    ssize_ = ssize;
  }

public:
  //! Inherit from asev::event_base.
  bool handle(thrctx_t& thrctx) override
  {
    ACTX_ASSERTS(!!hdr_);
    auto& pool = thrctx.get_event_pool<corctx_t>(corctx_make_t(thrctx.get_ev_service()));
    auto ev = cque::get_unique<corctx_t>(pool);
    ev->make_context(std::move(hdr_), ssize_);
    auto p = ev.release();
    p->resume();
    return true;
  }

  //! Inherit from cque::node_base
  void on_free() noexcept override
  {
    cque::node_base::on_free();
    hdr_ = handler_t();
  }

private:
  handler_t hdr_;
  coctx::stack_size ssize_;
};
}
}
