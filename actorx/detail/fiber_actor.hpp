//
// fiber_actor.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/actor/actor.hpp"
#include "actorx/detail/actor_base.hpp"
#include "actorx/detail/buffer_pool.hpp"
#include "actorx/detail/pack_event.hpp"

#include "actorx/asev/all.hpp"

#include <string>
#include <functional>


namespace actx
{
namespace detail
{
//! Fiber based actor.
class fiber_actor : public actor_base
{
  using base_t = actor_base;
  using actor_handler_t = std::function<void (actor)>;

public:
  fiber_actor(buffer_pool* pool, atom_t axid, int64_t timestamp)
    : base_t(pool, axid, timestamp)
  {
  }

public:
  void send(actor_id const& aid, message& msg) override
  {
    auto self_aid = base_t::get_actor_id();
    if (msg.get_sender() != self_aid)
    {
      msg.set_sender(self_aid);
    }

    if (aid.axid == axid_ && aid.timestamp == timestamp_)
    {
      actor_base* a = (actor_base*)aid.id;
      a->push_message(msg);
    }
  }

  void relay(actor_id const& aid, message const& msg) override
  {
    if (aid.axid == axid_ && aid.timestamp == timestamp_)
    {
      actor_base* a = (actor_base*)aid.id;
      a->push_message(msg);
    }
  }

  message recv(pattern const& patt) override
  {
    return nullmsg;
  }

  void quit(actor_exit aex = make<actor_exit>()) noexcept override
  {
  }

public:
  void run(asev::corctx_t& ctx, actor_id const& sire, actor_handler_t hdr)
  {
    try
    {
      actor self(this);
      hdr(self);
      quit();
    }
    catch (std::exception& ex)
    {
      // Quit with exception.
      auto aex = make<actor_exit>(exit_type::except, ex.what());
      quit(aex);
    }
  }

public:
  void push_message(message const& msg) noexcept override
  {
    ACTX_ASSERTS(!!snd_);
    auto& evs = snd_.get_ev_service();
    auto pk = evs.make_event<pack_event>(pack_make(pool_));
    pk->init(msg, this);
    snd_.async(pk);
  }

  void handle_message(message& msg) noexcept override
  {
    // Push to mailbox.
  }

private:
};
} // namespace detail
} // namespace actx
