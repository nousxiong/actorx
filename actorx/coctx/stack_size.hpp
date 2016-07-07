///
/// stack_size.hpp
///

#pragma once

#include "actorx/coctx/config.hpp"


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
}
