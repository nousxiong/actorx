//
// freer_base.hpp
//

#pragma once

#include "actorx/cque/config.hpp"


namespace cque
{
class node_base;
struct freer_base
{
  //! return false if pool full, user must handle node by themself
  virtual bool free(gsl::owner<node_base*>) noexcept = 0;
};
}
