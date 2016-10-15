//
// fiber_meta.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/actor/link_type.hpp"


namespace actx
{
//! Spawn fiber based actor's meta data.
struct fiber_meta
{
  fiber_meta()
    : lty_(link_type::no_link)
    , ssize_(0)
    , ssire_(false)
  {
  }

  fiber_meta& link(link_type lty)
  {
    lty_ = lty;
    return *this;
  }

  fiber_meta& stack_size(size_t ssize)
  {
    ssize_ = ssize;
    return *this;
  }

  fiber_meta& sync_sire(bool ssire)
  {
    ssire_ = ssire;
    return *this;
  }

  link_type lty_;
  size_t ssize_;
  bool ssire_;
};
}
