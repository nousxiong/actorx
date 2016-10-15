//
// message.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/actor/actor_id.hpp"
#include "actorx/actor/actor_exit.hpp"
#include "actorx/actor/msg_type.hpp"
#include "actorx/atom.hpp"
#include "actorx/detail/cow_buffer.hpp"

#include <adata/adata.hpp>

#include <vector>
#include <memory>
#include <type_traits>


namespace actx
{
class message
{
  using cbyte_span_t = gsl::span<byte_t const>;

public:
  message()
    : message(nullptr, "NULLTYPE", nullaid)
  {
  }

  explicit message(detail::buffer_pool* pool)
    : message(pool, "NULLTYPE", nullaid)
  {
  }

  template <size_t TypeSize>
  message(detail::buffer_pool* pool, char const (&type)[TypeSize])
    : message(pool, type, nullaid)
  {
  }

  message(detail::buffer_pool* pool, char const* type)
    : message(pool, type, nullaid)
  {
  }

  template <size_t TypeSize>
  message(detail::buffer_pool* pool, char const (&type)[TypeSize], actor_id const& sender)
    : type_(atom(type))
    , sender_(sender)
    , buffer_(pool)
  {
  }

  message(detail::buffer_pool* pool, char const* type, actor_id const& sender)
    : type_(dynatom(type))
    , sender_(sender)
    , buffer_(pool)
  {
  }

public:
  //! Set sender.
  void set_sender(actor_id const& sender) noexcept
  {
    sender_ = sender;
  }

  //! Get sender.
  actor_id const& get_sender() const noexcept
  {
    return sender_;
  }

  //! Set type.
  template <size_t TypeSize>
  void set_type(char const (&type)[TypeSize]) noexcept
  {
    type_ = atom(type);
  }

  //! Get type.
  atom_t get_type() const noexcept
  {
    return type_;
  }

  //! Get bytes.
  cbyte_span_t data() const noexcept
  {
    return buffer_.data();
  }

  void clear()
  {
    type_ = static_cast<atom_t>(msg_type::nulltype);
    sender_ = nullaid;
    buffer_.clear();
    shared_list_.clear();
  }

  //! Write given object to buffer.
  template <class T>
  message& put(T const& t)
  {
    auto size = adata::size_of(t);
    ACTX_ENSURES(buffer_.reserve(size) > 0);
    auto& stream = get_stream();
    auto s = buffer_.get_write();

    stream.set_write((unsigned char*)s.data(), s.size());
    adata::write(stream, t);
    buffer_.write(stream.write_length());
    return *this;
  }

  message& put(char const* str)
  {
    return put(std::string(str));
  }

  //! Write given shared_ptr to buffer.
  template <class T>
  message& put(std::shared_ptr<T> ptr)
  {
    int8_t index = (int8_t)shared_list_.size();
    put(index);
    shared_list_.push_back(std::move(ptr));
    return *this;
  }

  template <class T>
  T& get(T& t)
  {
    auto& stream = get_stream();
    auto s = buffer_.get_read();
    stream.set_read((unsigned char const*)s.data(), s.size());
    adata::read(stream, t);
    buffer_.read(stream.read_length());
    return t;
  }

  template <class T>
  T get()
  {
    T t;
    get(t);
    return t;
  }

  //! Read shared_ptr.
  template <class T>
  std::shared_ptr<T> get_raw()
  {
    int8_t index = get<int8_t>();
    ACTX_ENSURES(index >= 0 && index < (int)shared_list_.size())(index)(shared_list_.size());
    auto ptr = std::static_pointer_cast<T>(shared_list_[index]);
    shared_list_[index].reset();
    if (index + 1 == shared_list_.size())
    {
      shared_list_.clear();
    }
    return ptr;
  }

public:
  detail::buffer_pool* get_buffer_pool() const noexcept
  {
    return buffer_.get_buffer_pool();
  }

private:
  static adata::zero_copy_buffer& get_stream() noexcept
  {
    static thread_local adata::zero_copy_buffer stream;
    return stream;
  }

private:
  atom_t type_;
  actor_id sender_;
  detail::cow_buffer buffer_;

  //! Shared local list.
  std::vector<std::shared_ptr<void>> shared_list_;
};


template <>
struct maker<message>
{
  template <class... Args>
  static message create(Args... args)
  {
    return message(std::forward<Args>(args)...);
  }
};

//! To string of message.
static std::string to_string(message const& o)
{
  std::string str;
  str += "msg<";
  str += atom(o.get_type());
  str += ".";
  str += to_string(o.get_sender());
  str += ".";
  auto s = o.data();
  str += to_string(s.size());
  str += ">";
  return str;
}

//! For assertion.
template <>
struct format<message>
{
  static std::string convert(message const& o)
  {
    return to_string(o);
  }
};

static message const nullmsg = message();

static bool operator==(message const& lhs, message const& rhs)
{
  return
    lhs.get_sender() == rhs.get_sender() &&
    lhs.get_type() == rhs.get_type() &&
    lhs.get_buffer_pool() == rhs.get_buffer_pool()
    ;
}

static bool operator!=(message const& lhs, message const& rhs)
{
  return !(lhs == rhs);
}
}
