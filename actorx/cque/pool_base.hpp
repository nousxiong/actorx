///
/// pool_base.hpp
///

#pragma once

#include "actorx/cque/config.hpp"
#include "actorx/cque/freer_base.hpp"
#include "actorx/cque/node_base.hpp"

#include <memory>
#include <type_traits>


namespace cque
{
struct pool_base
  : public freer_base
{
  virtual ~pool_base() noexcept {}

  /// Try get a node from pool, if pool full, return nullptr.
  virtual gsl::owner<node_base*> get() noexcept = 0;
};

template <typename T>
struct pool_make
{
  static_assert(std::is_base_of<node_base, T>::value, "T must inhert from cque::node_base");
  gsl::owner<node_base*> operator()() const noexcept
  {
    return new (std::nothrow) T;
  }
};

template <typename T>
struct pool_delete
{
  static_assert(std::is_base_of<node_base, T>::value, "T must inhert from cque::node_base");
  void operator()(gsl::owner<T*> t) const noexcept
  {
    if (t != nullptr)
    {
      t->release();
    }
  }
};

/// Get an T object from pool.
template <typename T, typename Pool>
static gsl::owner<T*> get(Pool& pool) noexcept
{
  return gsl::narrow_cast<T*>(pool.get());
}

template <typename T, typename Pool>
std::shared_ptr<T> get_shared(Pool& pool) noexcept
{
  static_assert(std::is_base_of<pool_base, Pool>::value, "Pool must inhert from cque::pool_base");
  static_assert(std::is_base_of<node_base, T>::value, "T must inhert from cque::node_base");
  auto t = get<T>(pool);
  return std::move(std::shared_ptr<T>(t, pool_delete<T>()));
}

template <typename T, typename Pool>
std::unique_ptr<T, pool_delete<T>> get_unique(Pool& pool) noexcept
{
  static_assert(std::is_base_of<pool_base, Pool>::value, "Pool must inhert from cque::pool_base");
  static_assert(std::is_base_of<node_base, T>::value, "T must inhert from cque::node_base");
  auto t = get<T>(pool);
  return std::move(std::unique_ptr<T, pool_delete<T>>(t));
}
}
