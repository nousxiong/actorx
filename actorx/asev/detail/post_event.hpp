//
// post_event.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"

#include <functional>


namespace asev
{
namespace detail
{
//! Post event, for post an handler to run.
class post_event : public asev::event_base
{
  using handler_t = std::function<void (thrctx_t&)>;

public:
  //! Set handler to run.
  void set_handler(handler_t hdr)
  {
    hdr_ = std::move(hdr);
  }

public:
  //! Inherit from asev::event_base.
  bool handle(thrctx_t& thrctx) override
  {
    ACTX_ASSERTS(!!hdr_);
    hdr_(thrctx);
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
};
}
}
