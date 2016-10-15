//
// pack_event.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/actor/message.hpp"

#include "actorx/asev/all.hpp"
#include "actorx/cque/all.hpp"

#include <string>


namespace actx
{
namespace detail
{
//! Forward declare.
class buffer_pool;

//! Pack event.
class pack_event : public asev::event_base
{
public:
  explicit pack_event(buffer_pool* pool)
    : msg_(pool)
    , a_(nullptr)
  {
  }

  void init(message const& msg, actor_base* a)
  {
    msg_ = msg;
    a_ = a;
  }

  //! Inherit from asev::event_base.
  bool handle(thrctx_t&) override
  {
    a_->handle_message(msg_);
    return true;
  }

  void on_free() noexcept override
  {
    cque::node_base::on_free();
    msg_.clear();
    a_ = nullptr;
  }

private:
  message msg_;
  actor_base* a_;
};

struct pack_make
{
  explicit pack_make(buffer_pool* pool)
    : pool_(pool)
  {
  }

  gsl::owner<pack_event*> operator()() const noexcept
  {
    return new (std::nothrow) pack_event(pool_);
  }

  buffer_pool* pool_;
};
} // namespace detail
} // namespace actx
