//
// config.hpp
//

#pragma once

#include "actorx/user.hpp"

#include <gsl.h>


#ifndef ACTX_ENABLE_ASSERT
# if !defined(NDEBUG) || defined(ACTX_DEBUG)
#   define ACTX_ENABLE_ASSERT
# endif
#endif

#ifndef SPDLOG_DEBUG_ON
# ifdef ACTX_DEBUG
#   define SPDLOG_DEBUG_ON
# endif
#endif

#ifndef ACTX_SMALL_BUFFER_SIZE
# define ACTX_SMALL_BUFFER_SIZE 64
#endif // ACTX_SMALL_BUFFER_SIZE
