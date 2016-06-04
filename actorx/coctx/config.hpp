///
/// config.hpp
///

#pragma once

#include <gsl.h>


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define COCTX_WINDOWS 1
# if !defined(_WIN32_WINNT)
#   define _WIN32_WINNT 0x0502
# endif
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif
