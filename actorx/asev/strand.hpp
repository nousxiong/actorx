///
/// strand.hpp
///

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/ev_service.hpp"
#include "actorx/asev/detail/basic_strand.hpp"


namespace asev
{
using strand_t = detail::basic_strand<ev_service>;
}
