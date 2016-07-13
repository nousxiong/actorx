//
// all.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"
#include "actorx/asev/thrctx.hpp"
#include "actorx/asev/corctx.hpp"
#include "actorx/asev/affix_base.hpp"
#include "actorx/asev/ev_service.hpp"
#include "actorx/asev/strand.hpp"

#if defined(ACTORX_SPDLOG_DEBUG_ON) && defined(SPDLOG_DEBUG_ON)
# undef SPDLOG_DEBUG_ON
#endif // ACTORX_SPDLOG_DEBUG_ON
