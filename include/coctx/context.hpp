///
/// context.hpp
///

#pragma once

#include <coctx/config.hpp>

#include <functional>
#include <system_error>


namespace coctx
{
/// Coroutine stack size.
struct stack_size
{
  size_t size() const
  {
    return size_;
  }

private:
  size_t size_;
  friend stack_size make_stacksize(size_t);
};

/// Make stack size.
/**
 * @param size User given size, 0~max.
 * @return An stack_size struct, with fixed size.
 */
inline stack_size make_stacksize(size_t size = 0)
{
  stack_size ssize;
#if defined(COCTX_WINDOWS)
  /// On windows using fiber, and fiber stack size must be multiples of 64k(SYSTEM_INFO.dwAllocationGranularity)
  size_t constexpr alloc_granularity = 64 * 1024;
  ssize.size_ = (size / alloc_granularity + 1) * alloc_granularity;
#else
  size_t constexpr min_size = 64 * 1024;
  ssize.size_ = (std::max)(min_size, size);
#endif // defined
  return ssize;
}

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
#ifdef COCTX_WINDOWS
    : fiber_(nullptr)
#endif // COCTX_WINDOWS
    , null_(true)
  {
  }

  template <typename F>
  context(F&& f, stack_size ssize = make_stacksize())
    : ssize_(ssize.size())
#ifdef COCTX_WINDOWS
# if _WIN32_WINNT >= 0x0502
    , fiber_(CreateFiberEx(0, ssize_, FIBER_FLAG_FLOAT_SWITCH, context::run_fiber, this))
# else
    , fiber_(CreateFiber(ssize_, context::run_fiber, this))
# endif
#endif
    , hdr_(f)
    , null_(false)
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
#endif // COCTX_WINDOWS
  }

private:
#ifdef COCTX_WINDOWS
  static VOID CALLBACK run_fiber(void* arg)
  {
    auto self = (context*)arg;
    self->hdr_(*self);
  }
#endif

private:
  size_t ssize_;
#ifdef COCTX_WINDOWS
  gsl::owner<void*> fiber_;
#endif
  handler_t hdr_;
  bool null_;
};
} /// namespace coctx
