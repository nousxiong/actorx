//
// buffer.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/integer.hpp"
#include "actorx/detail/intrusive_ptr.hpp"

#include "actorx/cque/all.hpp"

#include <atomic>
#include <array>


namespace actx
{
namespace detail
{
//! Buffer of bytes.
class buffer : public cque::node_base
{
  using base_t = cque::node_base;
  using byte_span_t = gsl::span<byte_t>;
  using cbyte_span_t = gsl::span<byte_t const>;

  buffer() = delete;
  buffer(buffer const&) = delete;
  buffer& operator=(buffer const&) = delete;
  buffer(buffer&&) = delete;
  buffer& operator=(buffer&&) = delete;

public:
  explicit buffer(size_t size) noexcept
    : count_(0)
    , data_((byte_t*)std::malloc(size))
    , size_(size)
  {
    if (size_ > 0)
    {
      ACTX_ENSURES(data_ != nullptr)(size_);
    }
  }

  ~buffer() noexcept
  {
    if (data_ != nullptr)
    {
      std::free(data_);
    }
  }

public:
  //! Get data.
  byte_span_t data(size_t offset = 0) noexcept
  {
    if (offset >= size_)
    {
      return byte_span_t();
    }
    return gsl::as_span(data_ + offset, size_ - offset);
  }

  //! Get const data.
  cbyte_span_t data(size_t offset = 0) const noexcept
  {
    if (offset >= size_)
    {
      return cbyte_span_t();
    }
    return gsl::as_span(data_ + offset, size_ - offset);
  }

  //! Try resize buffer.
  /**
   * @param size New size of buffer.
   * @return new size, if 0 and size != 0 means err.
   */
  size_t resize(size_t size)
  {
    if (size > size_)
    {
      auto p = std::realloc(data_, size);
      if (p == nullptr)
      {
        return 0;
      }
      data_ = (byte_t*)p;
    }
    size_ = size;
    return size;
  }

  // For intrusive_ptr.
public:
  int use_count() const noexcept
  {
    return count_;
  }

  void incr_ref() noexcept
  {
    count_.fetch_add(1, std::memory_order_relaxed);
  }

  void decr_ref() noexcept
  {
    if (count_.fetch_sub(1, std::memory_order_release) == 1)
    {
      std::atomic_thread_fence(std::memory_order_acquire);
      base_t::release();
    }
  }

private:
  std::atomic_int count_;
  gsl::owner<byte_t*> data_;
  size_t size_;
};

//! An small static buffer.
template <size_t Capacity>
class small_buffer
{
  using byte_span_t = gsl::span<byte_t>;
  using cbyte_span_t = gsl::span<byte_t const>;

public:
  byte_span_t data(size_t offset = 0) noexcept
  {
    if (offset >= data_.size())
    {
      return byte_span_t();
    }
    return gsl::as_span(data_.data() + offset, data_.size() - offset);
  }

  cbyte_span_t data(size_t offset = 0) const noexcept
  {
    if (offset >= data_.size())
    {
      return cbyte_span_t();
    }
    return gsl::as_span(data_.data() + offset, data_.size() - offset);
  }

private:
  std::array<byte_t, Capacity> data_;
};

//! intrusive_ptr of buffer.
using buffer_ptr = intrusive_ptr<buffer>;

//! Buffer maker.
struct buffer_make
{
  explicit buffer_make(size_t size)
    : size_(size)
  {
  }

  gsl::owner<buffer*> operator()() const noexcept
  {
    return new (std::nothrow) buffer(size_);
  }

  size_t const size_;
};
} // namespace detail
} // namespace actx
