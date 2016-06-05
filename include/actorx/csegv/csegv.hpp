///
/// csegv.hpp
///

#pragma once

#include "actorx/csegv/config.hpp"
#ifdef _MSC_VER
# include "actorx/csegv/detail/windows.hpp"
#elif defined(CSEGV_WINDOWS) /// mingw
# include "actorx/csegv/detail/stack.hpp"
#else
# include "actorx/csegv/detail/posix.hpp"
#endif


namespace csegv
{
static void init(size_t stack_depth = 3, bool brief = true) noexcept
{
  detail::max_stack_depth(stack_depth);
  detail::is_brief_info(brief ? -1 : 1);

#ifdef _MSC_VER
  HANDLE hProcess = GetCurrentProcess();
  auto ret = SymInitialize(hProcess, NULL, TRUE);
  assert(ret);
#endif
}

/// Call a given function under protection.
template <typename F>
static void pcall(F&& f)
{
#ifdef _MSC_VER
  __try
  {
    f();
  }
  __except (detail::seh_filter(GetExceptionInformation()))
  {
    std::exit(102);
  }
#elif defined(CSEGV_WINDOWS) /// mingw
  f();
#else

  static thread_local detail::context ctx{false};
  detail::signal_filter(ctx);

  f();
#endif
}
} /// namespace csegv
