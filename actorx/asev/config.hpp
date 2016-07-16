//
// config.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/assertion.hpp"
#include "actorx/coctx/all.hpp"
#include "actorx/cque/all.hpp"
#include "actorx/csegv/all.hpp"


#ifndef ASEV_MAX_EV_SERVICE
# define ASEV_MAX_EV_SERVICE 64
#endif // ASEV_MAX_EV_SERVICE

#ifndef ASEV_MAX_AFFIX_LIST_DIVISION
# define ASEV_MAX_AFFIX_LIST_DIVISION 16
#endif // ASEV_MAX_AFFIX_LIST_DIVISION

#ifdef ACTORX_DEBUG
# ifndef SPDLOG_DEBUG_ON
#   define ACTORX_SPDLOG_DEBUG_ON
#   define SPDLOG_DEBUG_ON
# endif
#endif // ACTORX_DEBUG
