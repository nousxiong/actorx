//
// actor_id.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/atom.hpp"
#include "actorx/actor/actor_id.adl.h"
#include "actorx/make.hpp"

#include <string>


namespace actx
{
using actor_id = actorx::adl::actor_id;

//! Null actor_id.
static actor_id const nullaid = actor_id();

//! Maker of actor_id.
template <>
struct maker<actor_id>
{
  static actor_id create()
  {
    return actor_id();
  }

  template <size_t AxidSize>
  static actor_id create(char const (&type)[AxidSize], int64_t timestamp, int64_t id, int64_t sid)
  {
    return create(atom(type), timestamp, id, sid);
  }

  static actor_id create(int64_t axid, int64_t timestamp, int64_t id, int64_t sid)
  {
    actor_id aid;
    aid.axid = axid;
    aid.timestamp = timestamp;
    aid.id = id;
    aid.sid = sid;
    return aid;
  }
};

//! To string of actor_id.
static std::string to_string(actor_id const& o)
{
  std::string str;
  str += "aid<";
  str += atom(o.axid);
  str += ".";
  str += to_string(o.timestamp);
  str += ".";
  str += to_string(o.id);
  str += ".";
  str += to_string(o.sid);
  str += ">";
  return str;
}

//! For assertion.
template <>
struct format<actor_id>
{
  static std::string convert(actor_id const& o)
  {
    return to_string(o);
  }
};
} // namespace actx

namespace actorx
{
namespace adl
{
static bool operator==(actor_id const& lhs, actor_id const& rhs)
{
  return
    lhs.axid == rhs.axid &&
    lhs.timestamp == rhs.timestamp &&
    lhs.id == rhs.id &&
    lhs.sid == rhs.sid
    ;
}

static bool operator!=(actor_id const& lhs, actor_id const& rhs)
{
  return !(lhs == rhs);
}

static bool operator<(actor_id const& lhs, actor_id const& rhs)
{
  if (lhs.axid < rhs.axid)
  {
    return true;
  }
  else if (rhs.axid < lhs.axid)
  {
    return false;
  }

  if (lhs.timestamp < rhs.timestamp)
  {
    return true;
  }
  else if (rhs.timestamp < lhs.timestamp)
  {
    return false;
  }

  if (lhs.id < rhs.id)
  {
    return true;
  }
  else if (rhs.id < lhs.id)
  {
    return false;
  }

  if (lhs.sid < rhs.sid)
  {
    return true;
  }
  else if (rhs.sid < lhs.sid)
  {
    return false;
  }

  return false;
}
} // namespace adl
} // namespace actorx
