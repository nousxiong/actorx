///
/// eout.hpp
///

#pragma once

#include <asev/config.hpp>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <mutex>


namespace asev
{
namespace detail
{
struct cwriter
{
  std::mutex mtx_;
  fmt::MemoryWriter w_;
};
}
static void eout(fmt::CStringRef format_str, fmt::ArgList args)
{
  static detail::cwriter cw;
  std::unique_lock<std::mutex> lock(cw.mtx_);
  fmt::MemoryWriter& w = cw.w_;
  w.write(format_str, args);
  std::fwrite(w.data(), 1, w.size(), stdout);
  w.clear();
}

FMT_VARIADIC(void, eout, fmt::CStringRef)
}
