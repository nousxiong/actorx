//
// exit_type.hpp
//

#pragma once

#include "actorx/config.hpp"


namespace actx
{
//! Actor's exit type.
enum class exit_type : int8_t
{
  normal,
  except,
};
} // namespace actx
