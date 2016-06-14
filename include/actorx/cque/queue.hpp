///
/// queue.hpp
///

#pragma once

#include "actorx/cque/config.hpp"
#include "actorx/cque/node_base.hpp"
#include "actorx/cque/variant_ptr.hpp"

#include <memory>
#include <type_traits>


namespace cque
{
/// Single thread linked queue.
template <typename T>
class queue
{
  static_assert(std::is_base_of<node_base, T>::value, "T must inhert from cque::node_base");
  using node_t = node_base;

  queue(queue const&) = delete;
  queue& operator=(queue const&) = delete;
  queue(queue&&) = delete;
  queue& operator=(queue&&) = delete;

public:
  queue()
    : head_(nullptr)
    , tail_(nullptr)
  {
  }

  ~queue()
  {
    while (true)
    {
      auto e = pri_pop();
      if (e != nullptr)
      {
        e->destory();
      }
      else
      {
        break;
      }
    }
  }

public:
  bool empty() const
  {
    return head_ == nullptr;
  }

  /// Pop an element from head.
  gsl::owner<T*> pop() noexcept
  {
    auto e = pri_pop();
    if (e != nullptr)
    {
      ACTORX_ASSERTS(!e->shared());
      e->free_unique();
    }
    return e;
  }

  /// Push an element at tail.
  void push(gsl::owner<T*> e) noexcept
  {
    if (e == nullptr)
    {
      return;
    }

    ACTORX_ASSERTS(!e->shared());
    ACTORX_ASSERTS(!e->unique());
    pri_push(e);
  }

  /// Pop an shared_ptr element from head.
  /**
   * @note Only call in single-consumer thread.
   */
  std::shared_ptr<T> pop_shared() noexcept
  {
    auto e = pri_pop();
    if (e != nullptr)
    {
      ACTORX_ASSERTS(e->shared());
      ACTORX_ASSERTS(!e->unique());
      auto ptr = e->free_shared();
      ACTORX_ASSERTS(!!ptr);
      return std::move(std::static_pointer_cast<T>(ptr));
    }
    return std::shared_ptr<T>();
  }

  /// Push an shared_ptr element at tail.
  void push(std::shared_ptr<T> e) noexcept
  {
    if (!e)
    {
      return;
    }

    auto p = e.get();
    ACTORX_ASSERTS(!p->shared());
    ACTORX_ASSERTS(!p->unique());
    p->retain(std::move(e));
    pri_push(p);
  }

  /// Pop an unique_ptr element from head.
  /**
   * @note Only call in single-consumer thread.
   */
  template <typename D = std::default_delete<T>>
  std::unique_ptr<T, D> pop_unique(D d = D()) noexcept
  {
    auto e = pri_pop();
    if (e != nullptr)
    {
      ACTORX_ASSERTS(!e->shared());
      e->free_unique();
    }
    return std::move(std::unique_ptr<T, D>(e, d));
  }

  /// Push an unique_ptr element at tail.
  template <typename D>
  void push(std::unique_ptr<T, D>&& e) noexcept
  {
    if (!e)
    {
      return;
    }

    auto p = e.release();
    ACTORX_ASSERTS(!p->shared());
    ACTORX_ASSERTS(!p->unique());
    p->retain();
    pri_push(p);
  }

  /// Pop a variant_ptr element at head.
  template <typename D = std::default_delete<T>>
  variant_ptr<T, D> pop_variant(D d = D()) noexcept
  {
    auto e = pri_pop();
    if (e != nullptr)
    {
      if (e->shared())
      {
        ACTORX_ASSERTS(!e->unique());
        auto ptr = e->free_shared();
        ACTORX_ASSERTS(!!ptr);
        return std::move(variant_ptr<T, D>(std::static_pointer_cast<T>(ptr)));
      }
      else if (e->unique())
      {
        ACTORX_ASSERTS(!e->shared());
        e->free_unique();
        return std::move(variant_ptr<T, D>(std::unique_ptr<T, D>(e)));
      }
      else
      {
        ACTORX_ASSERTS(!e->unique());
        ACTORX_ASSERTS(!e->shared());
        return std::move(variant_ptr<T, D>(e));
      }
    }
    return std::move(variant_ptr<T, D>());
  }

private:
  gsl::owner<T*> pri_pop() noexcept
  {
    if (head_ == nullptr)
    {
      return nullptr;
    }

    auto e = head_;
    head_ = e->fetch_next();
    if (head_ == nullptr)
    {
      ACTORX_ASSERTS(e == tail_);
      tail_ = nullptr;
    }
    return (gsl::owner<T*>)e;
  }

  void pri_push(gsl::not_null<T*> e) noexcept
  {
    e->set_next(nullptr);
    if (tail_ == nullptr)
    {
      ACTORX_ASSERTS(head_ == nullptr);
      head_ = e;
      tail_ = e;
      return;
    }

    tail_->set_next(e);
    tail_ = e;
  }

private:
  gsl::owner<node_t*> head_;
  gsl::owner<node_t*> tail_;
};
}
