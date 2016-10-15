//
// make.hpp
//

#pragma once

#include "actorx/user.hpp"


namespace actx
{
template <typename T>
struct maker
{
};

template <class T, class... Args>
static T make(Args&&... args)
{
  return maker<T>::create(std::forward<Args>(args)...);
}
} // namespace actx
