//
// actor_exit.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/actor/actor_exit.adl.h"
#include "actorx/actor/exit_type.hpp"
#include "actorx/make.hpp"

#include <string>


namespace actx
{
using actor_exit = actorx::adl::actor_exit;

//! Maker of actor_exit.
template <>
struct maker<actor_exit>
{
  static actor_exit create()
  {
    return create(exit_type::normal, std::string());
  }

  static actor_exit create(exit_type type, std::string msg)
  {
    actor_exit aex;
    aex.type = static_cast<int8_t>(type);
    aex.errmsg = std::move(msg);
    return aex;
  }
};

//! To string of actor_id.
static std::string to_string(actor_exit const& o)
{
  std::string str;
  str += "aex<";
  str += to_string((int)o.type);
  str += ".";
  str += o.errmsg;
  str += ">";
  return str;
}

//! For assertion.
template <>
struct format<actor_exit>
{
  static std::string convert(actor_exit const& o)
  {
    return to_string(o);
  }
};
} // namespace actx
