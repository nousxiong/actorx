//
// tsegv_event.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/threv_base.hpp"

#include <list>
#include <functional>


namespace asev
{
namespace detail
{
//! Thread segv event,
//! For post an handler on all ev_service's background threads when catched segv to run.
class tsegv_event : public ::asev::threv_base
{
  using handler_t = std::function<void (thrctx_t&, std::list<csegv::stack_info> const&)>;

public:
  tsegv_event()
    : stack_info_list_(nullptr)
  {
  }

  //! Set handler to run.
  void set_handler(handler_t const& hdr)
  {
    hdr_ = hdr;
  }

  void set_stack_info_list(std::list<csegv::stack_info> const& stack_info_list)
  {
    stack_info_list_ = &stack_info_list;
  }

public:
  //! Inherit from asev::event_base.
  bool handle(thrctx_t& thrctx) override
  {
    ACTX_ASSERTS(!!hdr_);
    ACTX_ASSERTS(stack_info_list_ != nullptr);
    hdr_(thrctx, *stack_info_list_);
    return true;
  }

  //! Inherit from cque::node_base
  void on_free() noexcept override
  {
    cque::node_base::on_free();
    hdr_ = handler_t();
    stack_info_list_ = nullptr;
  }

private:
  handler_t hdr_;
  std::list<csegv::stack_info> const* stack_info_list_;
};
}
}
