//
// atom.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/detail/atom_val.hpp"

#include <string>
#include <functional>
#include <type_traits>
#include <cstdint>
#include <cstring>


namespace actx
{
//! Creates an atom from given string literal.
template <size_t Size>
static atom_t constexpr atom(char const (&str)[Size])
{
  // last character is the NULL terminator
  static_assert(Size <= 11, "only 10 characters are allowed");
  return detail::atom_val(str, 0) + (1LL << 60);
}

//! Creates an atom from given string.
static atom_t dynatom(gsl::czstring str)
{
  ACTX_ASSERTS(std::strlen(str) <= 11).msg("only 10 characters are allowed");
  return detail::atom_val(str, 0) + (1LL << 60);
}

//! Creates an atom from given string.
static atom_t dynatom(std::string const& str)
{
  ACTX_ASSERTS(str.size() <= 11).msg("only 10 characters are allowed");
  return detail::atom_val(str.c_str(), 0) + (1LL << 60);
}

//! Atom to string.
static std::string atom(atom_t val)
{
  return std::move(detail::atom_val(val - (1LL << 60)));
}
} // namespace actx
