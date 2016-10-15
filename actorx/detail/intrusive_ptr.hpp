//
// intrusive_ptr.hpp
//

#pragma once

#include "actorx/config.hpp"


namespace actx
{
namespace detail
{
//! Intrusive pointer
template <class T>
class intrusive_ptr
{
public:
  intrusive_ptr() noexcept
    : px_(nullptr)
  {
  }

  intrusive_ptr(T* p, bool add_ref = true)
    : px_(p)
  {
    if (px_ != nullptr && add_ref)
    {
      px_->incr_ref();
    }
  }

  intrusive_ptr(intrusive_ptr const& rhs)
    : px_(rhs.px_)
  {
    if (px_ != nullptr)
    {
      px_->incr_ref();
    }
  }

  ~intrusive_ptr()
  {
    if (px_ != nullptr)
    {
      px_->decr_ref();
    }
  }

  intrusive_ptr(intrusive_ptr&& rhs) noexcept
    : px_(rhs.px_)
  {
    rhs.px_ = nullptr;
  }

  intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept
  {
    intrusive_ptr(static_cast<intrusive_ptr&&>(rhs)).swap(*this);
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr const& rhs)
  {
    intrusive_ptr(rhs).swap(*this);
    return *this;
  }

  intrusive_ptr& operator=(T* rhs)
  {
    intrusive_ptr(rhs).swap(*this);
    return *this;
  }

  void reset() noexcept
  {
    intrusive_ptr().swap(*this);
  }

  void reset(T* rhs)
  {
    intrusive_ptr(rhs).swap(*this);
  }

  void reset(T* rhs, bool add_ref)
  {
    intrusive_ptr(rhs, add_ref).swap(*this);
  }

  T* get() const noexcept
  {
    return px_;
  }

  T* detach() noexcept
  {
    T* ret = px_;
    px_ = 0;
    return ret;
  }

  T& operator*() const noexcept
  {
    ACTX_ASSERTS(px_ != 0);
    return *px_;
  }

  T* operator->() const noexcept
  {
    ACTX_ASSERTS(px_ != 0);
    return px_;
  }

  operator bool() const noexcept
  {
    return px_ != nullptr;
  }

  bool operator!() const noexcept
  {
    return px_ == nullptr;
  }

  void swap(intrusive_ptr& rhs) noexcept
  {
    std::swap(px_, rhs.px_);
  }

private:
  T* px_;
};
} // namespace detail
} // namespace actx
