//
// node_base.hpp
//

#pragma once

#include "actorx/cque/config.hpp"
#include "actorx/cque/freer_base.hpp"

#include <memory>


namespace cque
{
//! node base
class node_base
{
  node_base(node_base&) = delete;
  node_base(node_base&&) = delete;
  node_base& operator=(node_base const&) = delete;
  node_base& operator=(node_base&&) = delete;
public:
  node_base() noexcept
    : next_(nullptr)
    , freer_(nullptr)
    , unique_(false)
  {
  }

  virtual ~node_base() noexcept
  {
  }

public:
  //! call after get from pool
  virtual void on_get(freer_base* freer) noexcept
  {
    next_ = nullptr;
    freer_ = freer;
    self_.reset();
    unique_ = false;
  }

  //! call before free to pool
  virtual void on_free() noexcept
  {
    next_ = nullptr;
    freer_ = nullptr;
    self_.reset();
    unique_ = false;
  }

protected:
  //! destroy self
  virtual void dispose() noexcept
  {
    delete this;
  }

public:
  node_base* get_next() const noexcept
  {
    return next_;
  }

  node_base* fetch_next() noexcept
  {
    auto n = next_;
    next_ = nullptr;
    return n;
  }

  void set_next(node_base* next) noexcept
  {
    next_ = next;
  }

  void release() noexcept
  {
    if (freer_ == nullptr || !freer_->free(this))
    {
      dispose();
    }
  }

  //! internal use
  //! for retain shared_ptr, only call this for inqueue
  template <typename T>
  void retain(std::shared_ptr<T>&& self) noexcept
  {
    static_assert(std::is_base_of<node_base, T>::value, "T must inhert from cque::node_base");
    ACTORX_ASSERTS(self.get() == this);
    self_ = self;
  }

  //! check if shared
  bool shared() const noexcept
  {
    return self_.get() != nullptr;
  }

  //! free self ptr
  std::shared_ptr<node_base> free_shared() noexcept
  {
    return std::move(self_);
  }

  //! retain unique_ptr
  void retain() noexcept
  {
    unique_ = true;
  }

  //! check if unique
  bool unique() const noexcept
  {
    return unique_;
  }

  //! free unique_ptr
  void free_unique() noexcept
  {
    unique_ = false;
  }

  void destory() noexcept
  {
    if (self_)
    {
      self_.reset();
    }
    else
    {
      release();
    }
  }

private:
  node_base* next_;
  freer_base* freer_;
  std::shared_ptr<node_base> self_;
  bool unique_;
};
}
