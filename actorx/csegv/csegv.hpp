//
// csegv.hpp
//

#pragma once

#include "actorx/csegv/config.hpp"
#include "actorx/csegv/stack_info.hpp"
#ifdef _MSC_VER
# include "actorx/csegv/detail/windows.hpp"
#elif !defined(CSEGV_WINDOWS)
# include "actorx/csegv/detail/posix.hpp"
#endif

#include <mutex>


namespace csegv
{
namespace detail
{
struct init_flag
{
  static init_flag& get()
  {
    static init_flag flag;
    return flag;
  }

  std::once_flag flag_;
private:
  init_flag() {}
};
}

//! Call a given function under protection.
template <typename F>
static void pcall(F&& f, handler_t h = handler_t(), size_t stack_depth = 3, bool brief = true)
{
  static thread_local bool inited{false};
#ifdef _MSC_VER
  auto& ctx = detail::context::get();
  ctx.stack_depth_ = stack_depth;
  ctx.brief_ = brief;
  ctx.h_ = std::move(h);

  if (!inited)
  {
    inited = true;
    auto& iflag = detail::init_flag::get();
    std::call_once(
      iflag.flag_, 
      []()
      {
        auto hProcess = GetCurrentProcess();
        auto ret = SymInitialize(hProcess, NULL, TRUE);
        assert(ret);
      });
  }

  detail::pcall_impl(f);

#elif defined(CSEGV_WINDOWS) // mingw
  f();
#else

  auto& ctx = detail::context::get();
  ctx.stack_depth_ = stack_depth;
  ctx.brief_ = brief;
  ctx.h_ = std::move(h);

  if (!inited)
  {
    inited = true;
    detail::signal_filter(ctx);
  }

  f();
#endif
}
} // namespace csegv
