//
// variant_ptr.hpp
//

#pragma once

#include "actorx/cque/config.hpp"

#include <memory>


namespace cque
{
template <typename T, class D = std::default_delete<T>>
class variant_ptr
{
  variant_ptr(variant_ptr const&) = delete;
  variant_ptr& operator=(variant_ptr const&) = delete;

public:
  //! Default constructor.
  variant_ptr() noexcept
    : raw_(nullptr)
  {
  }

  //! Construct from shared_ptr.
  explicit variant_ptr(std::shared_ptr<T>&& ptr) noexcept
    : shared_(ptr)
    , raw_(nullptr)
  {
  }

  //! Construct from unique_ptr.
  explicit variant_ptr(std::unique_ptr<T, D>&& ptr) noexcept
    : unique_(std::move(ptr))
    , raw_(nullptr)
  {
  }

  //! Construct from raw pointer with deleter.
  explicit variant_ptr(gsl::owner<T*>& ptr, D d = D()) noexcept
    : raw_(ptr)
    , d_(d)
  {
    ptr = nullptr;
  }

  //! Move constructor.
  variant_ptr(variant_ptr&& other) noexcept
    : shared_(std::move(other.shared_))
    , unique_(std::move(other.unique_))
    , raw_(other.raw_)
  {
    other.raw_ = nullptr;
  }

  ~variant_ptr() noexcept
  {
    if (raw_ != nullptr)
    {
      d_(raw_);
    }
  }

  //! Move assignment.
  variant_ptr& operator=(variant_ptr&& rhs) noexcept
  {
    if (this != &rhs)
    {
      shared_ = std::move(rhs.shared_);
      unique_ = std::move(rhs.unique_);
      raw_ = rhs.raw_;
      rhs.raw_ = nullptr;
    }
    return *this;
  }

public:
  //! Release ownership of shared_ptr, if any.
  std::shared_ptr<T> free_shared() noexcept
  {
    return std::move(shared_);
  }

  //! Release ownership of unique_ptr, if any.
  std::unique_ptr<T, D> free_unique() noexcept
  {
    return std::move(unique_);
  }

  //! Release ownership of raw pointer, if any.
  gsl::owner<T*> free_raw() noexcept
  {
    T* raw = raw_;
    raw_ = nullptr;
    return raw;
  }

  //! Operator for boolean.
  operator bool() const noexcept
  {
    return shared_.get() != nullptr || unique_.get() != nullptr || raw_ != nullptr;
  }

  //! Release ownership of all pointers and dispose them.
  void clear()
  {
    shared_.reset();
    unique_.reset();
    if (raw_ != nullptr)
    {
      d_(raw_);
      raw_ = nullptr;
    }
  }

  //! Operator ->.
  T* operator->() const noexcept
  {
    ACTORX_ASSERTS(shared_.get() != nullptr || unique_.get() != nullptr || raw_ != nullptr);
    if (shared_)
    {
      return shared_.get();
    }
    else if (unique_)
    {
      return unique_.get();
    }
    else
    {
      return raw_;
    }
  }

  //! Operator *.
  T& operator*() const noexcept
  {
    ACTORX_ASSERTS(shared_.get() != nullptr || unique_.get() != nullptr || raw_ != nullptr);
    if (shared_)
    {
      return *shared_;
    }
    else if (unique_)
    {
      return *unique_;
    }
    else
    {
      return *raw_;
    }
  }

  //! Get pointer may hold by pointers(shared_ptr, unique_ptr or raw).
  T* get() const noexcept
  {
    if (shared_)
    {
      return shared_.get();
    }
    else if (unique_)
    {
      return unique_.get();
    }
    else
    {
      return raw_;
    }
  }

private:
  std::shared_ptr<T> shared_;
  std::unique_ptr<T, D> unique_;
  gsl::owner<T*> raw_;
  D d_;
};

//! Make variant ptr from shared_ptr.
template <typename T>
static variant_ptr<T> make_variant(std::shared_ptr<T> ptr) noexcept
{
  return std::move(variant_ptr<T>(std::move(ptr)));
}

//! Make variant ptr from unique_ptr.
template <typename T, typename D = std::default_delete<T>>
static variant_ptr<T, D> make_variant(std::unique_ptr<T, D>&& ptr) noexcept
{
  return std::move(variant_ptr<T, D>(std::move(ptr)));
}

//! Make variant ptr from raw pointer with deleter.
template <typename T, typename D = std::default_delete<T>>
static variant_ptr<T, D> make_variant(gsl::owner<T*>& ptr, D d = D()) noexcept
{
  return std::move(variant_ptr<T, D>(ptr, d));
}
}
