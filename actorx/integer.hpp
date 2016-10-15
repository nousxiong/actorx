//
// integer.hpp
//

#pragma once

#include "actorx/user.hpp"

#include <string>
#include <array>
#include <cstdio>
#include <cstdint>


namespace actx
{
using byte_t = ::int8_t;
using intstr_t = std::array<char, 32>;

//! Integer to string
template <class I>
static std::string to_string(I i, char const* fmt)
{
  intstr_t str{"\0"};
  auto n = std::snprintf(str.data(), str.size(), fmt, i);
  return std::string(str.data(), n);
}

static std::string to_string(int i)
{
  return to_string(i, "%d");
}

static std::string to_string(unsigned int i)
{
  return to_string(i, "%u");
}

static std::string to_string(long i)
{
  return to_string(i, "%ld");
}

static std::string to_string(unsigned long i)
{
  return to_string(i, "%lu");
}

static std::string to_string(long long i)
{
  return to_string(i, "%lld");
}

static std::string to_string(unsigned long long i)
{
  return to_string(i, "%llu");
}
} // namespace actx
