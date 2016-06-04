///
/// event_base.hpp
///

#pragma once

#include "actorx/asev/config.hpp"


namespace asev
{
/// Forward declares.
class ev_service;

namespace detail
{
template <class>
class basic_thrctx;
}

/// Event base class.
struct event_base : public cque::node_base
{
  using thrctx_t = detail::basic_thrctx<ev_service>;

  event_base() {}
  virtual ~event_base() {}

  /// Handle this event.
  /**
   * @return true will release event, false leave it to user.
   * @note If throw a exception event will be released.
   */
  virtual bool handle(thrctx_t&) { return true; }
};
}
