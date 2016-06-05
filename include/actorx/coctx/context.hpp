///
/// context.hpp
///

#pragma once

#include "actorx/coctx/config.hpp"
#include "actorx/coctx/stack_size.hpp"
#ifndef COCTX_WINDOWS
# include "actorx/coctx/detail/posix.hpp"
#else
# include <windows.h>
#endif // COCTX_WINDOWS

#include <functional>
#include <system_error>


namespace coctx
{
/// Coroutine's context.
class context
{
  using handler_t = std::function<void (context&)>;

  context(context const&) = delete;
  context& operator=(context const&) = delete;
  context(context&&) = delete;
  context& operator=(context&&) = delete;

public:
  context()
    : ssize_(0)
#ifdef COCTX_WINDOWS
    , null_(true)
    , fiber_(nullptr)
#endif // COCTX_WINDOWS
  {
  }

  template <typename F>
  context(F f, stack_size ssize = make_stacksize())
    : ssize_(ssize.size())
#ifdef COCTX_WINDOWS
    , null_(false)
# if _WIN32_WINNT >= 0x0502
    , fiber_(CreateFiberEx(0, ssize_, FIBER_FLAG_FLOAT_SWITCH, context::run_fiber, this))
# else
    , fiber_(CreateFiber(ssize_, context::run_fiber, this))
# endif
#else
    , stk_(ssize_)
    , asmctx_(detail::make_coctx(stk_, context::run_asm, this))
#endif
    , hdr_(std::forward<F>(f))
  {
#ifdef COCTX_WINDOWS
    if (fiber_ == nullptr)
    {
      DWORD err = GetLastError();
      auto errc = std::errc::device_or_resource_busy;
      if (err == ERROR_NOT_ENOUGH_MEMORY)
      {
        errc = std::errc::not_enough_memory;
      }
      throw std::system_error(std::make_error_code(errc), "create fiber failed!");
    }
#endif
  }

  ~context()
  {
#ifdef COCTX_WINDOWS
    if (!null_ && fiber_ != nullptr)
    {
      DeleteFiber(fiber_);
    }
#endif // COCTX_WINDOWS
  }

public:
  inline size_t get_stack_size() const
  {
    return ssize_;
  }

  /// Jump to given context.
  /**
   * @param to_ctx Context that will jump to.
   */
  void jump(context& to_ctx)
  {
#ifdef COCTX_WINDOWS
    if (fiber_ == nullptr)
    {
      fiber_ = GetCurrentFiber();
      if (fiber_ == nullptr || fiber_ == (void*)0x1e00)
      {
        fiber_ = ConvertThreadToFiber(nullptr);
      }
    }
    SwitchToFiber(to_ctx.fiber_);
#else
    coctx_transfer(&asmctx_, &to_ctx.asmctx_);
#endif // COCTX_WINDOWS
  }

private:
#ifdef COCTX_WINDOWS
  static VOID CALLBACK run_fiber(void* arg)
  {
    auto self = (context*)arg;
    self->hdr_(*self);
  }
#else
  static void run_asm(void* arg)
  {
    auto self = (context*)arg;
    self->hdr_(*self);
  }
#endif

private:
  size_t ssize_;
#ifdef COCTX_WINDOWS
  bool null_;
  gsl::owner<void*> fiber_;
#else
  /// must be at offset 0.
  detail::stack stk_;
  coctx_asmctx asmctx_;
#endif
  handler_t hdr_;
};
} /// namespace coctx
