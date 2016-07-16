//
// config.hpp
//

#pragma once

#include "actorx/user.hpp"

#include <gsl.h>

#ifndef ACTORX_ENABLE_ASSERT
# if !defined(NDEBUG) || defined(ACTORX_DEBUG)
#   define ACTORX_ENABLE_ASSERT
# endif
#endif

#ifndef SPDLOG_DEBUG_ON
# ifdef ACTORX_DEBUG
#   define SPDLOG_DEBUG_ON
# endif
#endif
