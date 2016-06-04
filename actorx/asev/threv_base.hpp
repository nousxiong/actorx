///
/// threv_base.hpp
///

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"


namespace asev
{
/// Thread event base class.
struct threv_base : public event_base
{
  threv_base() {}
  virtual ~threv_base() {}
};
}
