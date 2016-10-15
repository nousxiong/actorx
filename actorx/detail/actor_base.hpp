//
// actor_base.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/actor/actor_id.hpp"
#include "actorx/actor/actor_exit.hpp"
#include "actorx/actor/message.hpp"
#include "actorx/atom.hpp"
#include "actorx/make.hpp"

#include "actorx/cque/all.hpp"

#include <string>


namespace actx
{
namespace detail
{
class buffer_pool;
//! Actor base.
class actor_base : public cque::node_base
{
public:
  actor_base(buffer_pool* pool, atom_t axid, int64_t timestamp)
    : pool_(pool)
    , axid_(axid)
    , timestamp_(timestamp)
    , aid_(nullaid)
  {
  }

  virtual ~actor_base()
  {
  }

public:

  buffer_pool* get_buffer_pool() const noexcept
  {
    return pool_;
  }

  atom_t get_axid() const noexcept
  {
    return axid_;
  }

  int64_t get_timestamp() const noexcept
  {
    return timestamp_;
  }

  actor_id const& get_actor_id() const noexcept
  {
    return aid_;
  }

  asev::strand_t get_strand() const noexcept
  {
    return snd_;
  }

  virtual void send(actor_id const& aid, message& msg) = 0;

  virtual void relay(actor_id const& aid, message const& msg) = 0;

  virtual message recv(pattern const& patt) = 0;

  virtual void quit(actor_exit aex = make<actor_exit>()) noexcept = 0;

public:
  void init(actor_id const& aid, asev::strand_t const& snd) noexcept
  {
    aid_ = aid;
    snd_ = snd;
  }

  virtual void push_message(message const& msg) noexcept = 0;
  virtual void handle_message(message& msg) noexcept = 0;

  void on_free() noexcept override
  {
    cque::node_base::on_free();
    aid_ = nullaid;
    snd_ = asev::strand_t();
  }

protected:
  buffer_pool* pool_;
  atom_t const axid_;
  int64_t const timestamp_;

  // Basic info.
  actor_id aid_;
  asev::strand_t snd_;
};

//! Actor maker.
template <class Actor>
struct actor_make
{
  actor_make(buffer_pool* pool, atom_t axid, int64_t timestamp)
    : pool_(pool)
    , axid_(axid)
    , timestamp_(timestamp)
  {
  }

  gsl::owner<Actor*> operator()() const noexcept
  {
    return new (std::nothrow) Actor(pool_, axid_, timestamp_);
  }

  buffer_pool* pool_;
  atom_t const axid_;
  int64_t const timestamp_;
};
} // namespace detail
} // namespace actx
