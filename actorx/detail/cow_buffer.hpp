//
// cow_buffer.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/detail/buffer_pool.hpp"
#include "actorx/detail/buffer.hpp"

#include <atomic>
#include <cstring>


namespace actx
{
namespace detail
{
//! Copy-on-write buffer.
class cow_buffer
{
  using byte_span_t = gsl::span<byte_t>;
  using cbyte_span_t = gsl::span<byte_t const>;

public:
  explicit cow_buffer(buffer_pool* pool)
    : pool_(pool)
    , is_small_(true)
    , write_size_(0)
    , read_size_(0)
  {
  }

  cow_buffer(cow_buffer const& other)
    : pool_(other.pool_)
    , is_small_(other.is_small_)
    , write_size_(other.write_size_)
    , read_size_(0)
  {
    if (is_small_)
    {
      copy_small(other);
    }
    else
    {
      large_ = other.large_;
    }
  }

  cow_buffer(cow_buffer&& other)
    : pool_(other.pool_)
    , is_small_(other.is_small_)
    , write_size_(other.write_size_)
    , read_size_(0)
  {
    if (is_small_)
    {
      copy_small(other);
    }
    else
    {
      large_ = std::move(other.large_);
    }

    other.is_small_ = true;
    other.write_size_ = 0;
    other.read_size_ = 0;
  }

  cow_buffer& operator=(cow_buffer const& rhs)
  {
    if (this == &rhs)
    {
      return *this;
    }

    read_size_ = 0;
    if (rhs.is_small())
    {
      if (!is_small())
      {
        large_.reset();
      }

      copy_small(rhs);
    }
    else
    {
      ACTX_ASSERTS(rhs.large_);
      large_ = rhs.large_;
    }

    is_small_ = rhs.is_small_;
    write_size_ = rhs.write_size_;
    return *this;
  }

  cow_buffer& operator=(cow_buffer&& rhs)
  {
    if (this == &rhs)
    {
      return *this;
    }

    read_size_ = 0;
    if (rhs.is_small())
    {
      if (!is_small())
      {
        large_.reset();
      }

      copy_small(rhs);
    }
    else
    {
      ACTX_ASSERTS(rhs.large_);
      large_ = std::move(rhs.large_);
    }

    is_small_ = rhs.is_small_;
    write_size_ = rhs.write_size_;

    rhs.is_small_ = true;
    rhs.read_size_ = 0;
    rhs.write_size_ = 0;
    return *this;
  }

public:
  bool is_small() const noexcept
  {
    return is_small_;
  }

  cbyte_span_t data() const noexcept
  {
    if (is_small())
    {
      return small_.data();
    }
    else
    {
      return large_->data();
    }
  }

  byte_span_t get_write() noexcept
  {
    if (is_small())
    {
      return small_.data(write_size_);
    }
    else
    {
      return large_->data(write_size_);
    }
  }

  cbyte_span_t get_read() const noexcept
  {
    if (is_small())
    {
      return small_.data(read_size_);
    }
    else
    {
      return large_->data(read_size_);
    }
  }

  size_t write(size_t size) noexcept
  {
    if (size == 0)
    {
      return size;
    }

    auto s = is_small() ? small_.data() : large_->data();
    if (write_size_ + size > s.size())
    {
      return 0;
    }

    write_size_ += size;
    return size;
  }

  size_t read(size_t size) noexcept
  {
    if (size == 0)
    {
      return size;
    }

    if (read_size_ + size > write_size_)
    {
      return 0;
    }

    read_size_ += size;
    return size;
  }

//  //! Copy given byte span to buffer.
//  /**
//   * @param src Byte span to copy from.
//   * @return writed size, if 0 and src.size() != 0, means err.
//   */
//  size_t write(cbyte_span_t src) noexcept
//  {
//    if (src.size() == 0)
//    {
//      return 0;
//    }
//
//    auto size = src.size();
//    if (reserve(size) == 0)
//    {
//      return 0;
//    }
//
//    auto dest = is_small() ? small_.data(write_size_) : large_->data(write_size_);
//    std::memcpy(dest.data(), src.data(), size);
//    write_size_ += size;
//    return size;
//  }

//  //! Copy buffer to given byte span.
//  /**
//   * @param dest Byte span to copy to.
//   * @return actually read size, if 0 and dest.size() != 0, means err;
//   */
//  size_t read(byte_span_t dest) noexcept
//  {
//    if (dest.size() == 0)
//    {
//      return 0;
//    }
//
//    auto size = dest.size();
//    if (read_size_ + size > write_size_)
//    {
//      return 0;
//    }
//
//    auto src = is_small() ? small_.data(read_size_) : large_->data(read_size_);
//    std::memcpy(dest.data(), src.data(), size);
//    read_size_ += size;
//    return size;
//  }

  //! Reserve given grow size.
  /**
   * @param size Growing size.
   * @return actually grow size, if 0 and size != 0 means err.
   */
  size_t reserve(size_t size) noexcept
  {
    if (size == 0)
    {
      return size;
    }

    auto grow_size = grow_normalize(size);
    if (is_small())
    {
      auto buf = small_.data();
      if (size + write_size_ > buf.size())
      {
        // Change to large_.
        large_ = pool_->get(grow_size + write_size_);
        bufcpy(*large_, small_, write_size_);
        is_small_ = false;
      }
      else
      {
        // When small buffer, no normalize.
        grow_size = size;
      }
    }
    else
    {
      ACTX_ASSERTS(!!large_);
      auto new_size = grow_size + write_size_;
      if (large_->use_count() > 1)
      {
        // Copy-on-write.
        auto large = pool_->get(new_size);
        bufcpy(*large, *large_, write_size_);
        large_ = large;
      }
      else
      {
        large_->resize(new_size);
      }
    }
    return grow_size;
  }

  void clear()
  {
    is_small_ = true;
    large_.reset();
    write_size_ = 0;
    read_size_ = 0;
  }

  buffer_pool* get_buffer_pool() const noexcept
  {
    return pool_;
  }

private:
  //! Copy small buffer.
  void copy_small(cow_buffer const& rhs)
  {
    if (rhs.write_size_ == 0)
    {
      return;
    }

    bufcpy(small_, rhs.small_, rhs.write_size_);
  }

  //! Copy buffer.
  template <class Dest, class Src>
  static void bufcpy(Dest& dest, Src const& src, size_t len)
  {
    std::memcpy(dest.data().data(), src.data().data(), len);
  }

  //! Get normalized grow size.
  /**
   * @param size New grow size.
   * @return normalized size.
   */
  size_t grow_normalize(size_t size) noexcept
  {
    if (size < ACTX_SMALL_BUFFER_SIZE)
    {
      size = ACTX_SMALL_BUFFER_SIZE;
    }
    return size;
  }

private:
  buffer_pool* pool_;
  bool is_small_;
  small_buffer<ACTX_SMALL_BUFFER_SIZE> small_;
  buffer_ptr large_;
  size_t write_size_;
  size_t read_size_;
};
} // namespace detail
} // namespace actx
