///
/// config.hpp
///

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/ev_service.hpp"
#include "actorx/asev/detail/basic_corctx.hpp"


namespace asev
{
using corctx_t = detail::basic_corctx<ev_service>;
}
