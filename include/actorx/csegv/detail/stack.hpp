///
/// stack.hpp
///

#pragma once

#include "actorx/csegv/config.hpp"
#include "actorx/csegv/stack_info.hpp"

#include <string>
#include <iostream>
#include <cassert>


namespace csegv
{
namespace detail
{
static size_t max_stack_depth(size_t size = 0)
{
  static size_t depth(3);
  if (size > 0)
  {
    depth = size;
  }
  return depth;
}
} /// namespace detail
} /// namespace csegv
