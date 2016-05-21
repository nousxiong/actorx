///
/// strand.hpp
///

#pragma once

#include <asev/config.hpp>
#include <asev/ev_service.hpp>
#include <asev/detail/basic_strand.hpp>


namespace asev
{
using strand_t = detail::basic_strand<ev_service>;
}
