//
// atom_val.hpp
//

#pragma once

#include "actorx/config.hpp"

#include <string>
#include <cstdint>


namespace actx
{
using atom_t = int64_t;
static atom_t const nullatom = -1;

namespace detail
{
namespace
{
// Encodes ASCII characters to 6bit encoding.
constexpr unsigned char encoding_table[] =
{
  /*     ..0 ..1 ..2 ..3 ..4 ..5 ..6 ..7 ..8 ..9 ..A ..B ..C ..D ..E ..F  */
  /* 0.. */  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /* 1.. */  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /* 2.. */  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /* 3.. */  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 0,  0,  0,  0,  0,  0,
  /* 4.. */  0, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  /* 5.. */ 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,  0,  0,  0,  0, 37,
  /* 6.. */  0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
  /* 7.. */ 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,  0,  0,  0,  0,  0 };

// Decodes 6bit characters to ASCII.
constexpr char decoding_table[] =
  " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
} // namespace <anonymous>

static atom_t constexpr next_interim(atom_t current, size_t char_code)
{
//  return (current << 6) | encoding_table[(char_code <= 0x7F) ? char_code : 0];
  return current * 64 + encoding_table[(char_code <= 0x7F) ? char_code : 0];
}

static atom_t constexpr atom_val(char const* cstr, atom_t interim = 0)
{
  return (*cstr == '\0') ?
    interim :
    atom_val(cstr + 1,
      next_interim(interim, static_cast<size_t>(*cstr)));
}

static std::string atom_val(atom_t av)
{
  std::string str;
  std::string::value_type buf[21] = {0};
  size_t pos = 19;
  while (av != 0)
  {
    buf[pos--] = decoding_table[av % 64];
    av /= 64;
  }
  ++pos;
  str.assign(buf + pos, 20 - pos);
  return std::move(str);
}
} // namespace detail
} // namespace actx
