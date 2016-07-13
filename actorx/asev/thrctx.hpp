//
// config.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/ev_service.hpp"
#include "actorx/asev/detail/basic_thrctx.hpp"


namespace asev
{
using thrctx_t = detail::basic_thrctx<ev_service>;
}
