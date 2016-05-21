///
/// config.hpp
///

#pragma once

#include <asev/config.hpp>
#include <asev/ev_service.hpp>
#include <asev/detail/basic_corctx.hpp>

namespace asev
{
using corctx_t = detail::basic_corctx<ev_service>;
}
