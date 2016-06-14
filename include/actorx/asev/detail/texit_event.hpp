///
/// texit_event.hpp
///

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/threv_base.hpp"

#include <functional>


namespace asev
{
namespace detail
{
/// Thread exit event,
/// For post an handler on all ev_service's background threads exit to run.
class texit_event : public ::asev::threv_base
{
  using handler_t = std::function<void (thrctx_t&)>;

public:
  /// Set handler to run.
  void set_handler(handler_t const& hdr)
  {
    hdr_ = hdr;
  }

public:
  /// Inherit from asev::event_base.
  bool handle(thrctx_t& thrctx) override
  {
    ACTORX_ASSERTS(!!hdr_);
    hdr_(thrctx);
    return true;
  }

  /// Inherit from cque::node_base
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
