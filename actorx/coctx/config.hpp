//
// config.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/assertion.hpp"


#if defined(__WINDOWS__) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
# define COCTX_WINDOWS 1
# ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0700
# endif
# define WIN32_LEAN_AND_MEAN
#endif
