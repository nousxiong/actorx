///
/// csegv.hpp
///

#pragma once

#include "actorx/csegv/config.hpp"
#ifdef CSEGV_WINDOWS
# include "actorx/csegv/detail/windows.hpp"
#else
# include "actorx/csegv/detail/posix.hpp"
#endif


namespace csegv
{
static void init(size_t stack_depth = 3, bool brief = true) noexcept
{
  detail::max_stack_depth(stack_depth);
  detail::is_brief_info(brief ? -1 : 1);

#ifdef CSEGV_WINDOWS
  HANDLE hProcess = GetCurrentProcess();
  auto ret = SymInitialize(hProcess, NULL, TRUE);
  assert(ret);
#endif
}

/// Call a given function under protection.
template <typename F>
static void pcall(F&& f)
{
#ifdef CSEGV_WINDOWS
  __try
  {
    f();
  }
  __except (detail::seh_filter(GetExceptionInformation()))
  {
    std::exit(102);
  }
#else

  static thread_local detail::context ctx{false};
  detail::signal_filter(ctx);

  f();
#endif
}
} /// namespace csegv
