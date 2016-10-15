//
// actor.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/actor/actor_id.hpp"
#include "actorx/actor/pattern.hpp"
#include "actorx/actor/message.hpp"
#include "actorx/actor/actor_exit.hpp"
#include "actorx/atom.hpp"
#include "actorx/make.hpp"
#include "actorx/detail/actor_base.hpp"

#include <string>


namespace actx
{
//! Actor proxy.
class actor
{
public:
  actor()
    : a_(nullptr)
  {
  }

  explicit actor(detail::actor_base* a)
    : a_(a)
  {
  }

  actor(actor&& other)
    : a_(other.a_)
  {
    other.a_ = nullptr;
  }

  actor(actor const& other)
    : a_(other.a_)
  {
  }

  actor& operator=(actor&& rhs)
  {
    if (this == &rhs)
    {
      return *this;
    }

    a_ = rhs.a_;
    rhs.a_ = nullptr;
    return *this;
  }

  actor& operator=(actor const& rhs)
  {
    if (this == &rhs)
    {
      return *this;
    }

    a_ = rhs.a_;
    return *this;
  }

public:
  inline actor_id const& get_actor_id() const noexcept
  {
    return a_->get_actor_id();
  }

  template <size_t TypeSize, class... Args>
  inline void send(actor_id const& aid, char const (&type)[TypeSize], Args&&... args)
  {
    auto msg = make<message>(a_->get_buffer_pool(), type, a_->get_actor_id());
    pack_arg(msg, std::forward<Args>(args)...);
    a_->send(aid, msg);
  }

  inline void send(actor_id const& aid, message& msg)
  {
    a_->send(aid, msg);
  }

  inline void relay(actor_id const& aid, message const& msg)
  {
    a_->relay(aid, msg);
  }

  template <class... Types>
  inline message recv(Types&&... types)
  {
    return a_->recv(pattern().match(std::forward<Types>(types)...));
  }

  inline message recv(pattern patt)
  {
    return a_->recv(patt);
  }

  void quit()
  {
    a_->quit();
  }

  void quit(actor_exit aex)
  {
    a_->quit(aex);
  }

public:
  //! Get actor, internal use.
  detail::actor_base* get_actor() const noexcept
  {
    return a_;
  }

  asev::strand_t get_strand() const noexcept
  {
    return a_->get_strand();
  }

private:
  void pack_arg(message&)
  {
  }

  template <class Arg>
  void pack_arg(message& msg, Arg&& arg)
  {
    msg.put(arg);
  }

  template <class Arg, class... Args>
  void pack_arg(message& msg, Arg&& arg, Args&&... args)
  {
    msg.put(arg);
    pack_arg(msg, std::forward<Args>(args)...);
  }

private:
  detail::actor_base* a_;
};

static actor const nullactor = actor();

static bool operator==(actor const& lhs, actor const& rhs)
{
  return lhs.get_actor() == rhs.get_actor();
}

static bool operator!=(actor const& lhs, actor const& rhs)
{
  return !(lhs == rhs);
}
} // namespace actx
