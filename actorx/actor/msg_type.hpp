//
// msg_type.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/atom.hpp"


namespace actx
{
//! Actor's messsage type.
enum class msg_type : atom_t
{
  exit = atom("EXIT"),
  nulltype = atom("NULLTYPE"),
};
} // namespace actx
