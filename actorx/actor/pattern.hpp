//
// pattern.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/actor/actor_id.hpp"
#include "actorx/actor/msg_type.hpp"

#include <chrono>
#include <vector>


namespace actx
{
//! Spawn coroutine based actor's meta data.
struct pattern
{
  template <class... Types>
  pattern& match(Types&&... types)
  {
    matches_.reserve(matches_.size() + sizeof...(types));
    return push_match(std::forward<Types>(types)...);
  }

  template <class... Aids>
  pattern& guard(Aids&&... aids)
  {
    guards_.reserve(guards_.size() + sizeof...(aids));
    return push_guard(std::forward<Aids>(aids)...);
  }

  template <typename Rep, typename Period>
  pattern& after(std::chrono::duration<Rep, Period> dur)
  {
    after_ = dur;
    return *this;
  }

  std::vector<atom_t> matches_;
  std::vector<actor_id> guards_;
  std::chrono::milliseconds after_;

private:
  template <size_t TypeSize>
  pattern& push_match(char const (&type)[TypeSize])
  {
    matches_.push_back(atom(type));
    return *this;
  }

  pattern& push_match(msg_type type)
  {
    matches_.push_back(static_cast<atom_t>(type));
    return *this;
  }

  template <size_t TypeSize, class... Types>
  pattern& push_match(char const (&type)[TypeSize], Types&&... types)
  {
    matches_.push_back(atom(type));
    return push_match(std::forward<Types>(types)...);
  }

  pattern& push_guard(actor_id const& aid)
  {
    guards_.push_back(aid);
    return *this;
  }

  template <class... Aids>
  pattern& push_guard(actor_id const& aid, Aids&&... aids)
  {
    guards_.push_back(aid);
    return push_guard(std::forward<Aids>(aids)...);
  }
};
}
