///
/// config.hpp
///

#pragma once

#include "actorx/user.hpp"

#include <gsl.h>


#ifndef CQUE_CACHE_LINE_SIZE
# define CQUE_CACHE_LINE_SIZE 64
#endif // CQUE_CACHE_LINE_SIZE

/// Ensure occupy entire cache(s) line.
#define CQUE_CACHE_ALIGNED_VAR(type_name, var) \
  type_name var; \
  char pad_##var[(sizeof(type_name)/CQUE_CACHE_LINE_SIZE + 1)*CQUE_CACHE_LINE_SIZE - sizeof(type_name)];
