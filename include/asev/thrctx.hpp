///
/// config.hpp
///

#pragma once

#include <asev/config.hpp>
#include <asev/ev_service.hpp>
#include <asev/detail/basic_thrctx.hpp>

namespace asev
{
using thrctx_t = detail::basic_thrctx<ev_service>;
}
