///
/// basic_thrctx.hpp
///

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"
#include "actorx/asev/affix_base.hpp"

#include <list>
#include <memory>
#include <typeinfo>
#include <typeindex>


namespace asev
{
namespace detail
{
/// Forward declares.
template <class>
class basic_corctx;

class worker;


template <class EvService>
class basic_thrctx
{
  struct event_pool
  {
    explicit event_pool(std::type_index&& tyidx)
      : tyidx_(tyidx)
    {
    }

    std::type_index tyidx_;
    std::unique_ptr<cque::pool_base> pool_;
  };

  using corctx_t = basic_corctx<EvService>;

public:
  basic_thrctx(EvService& evs, size_t index)
    : evs_(evs)
    , index_(index)
    , curr_worker_(nullptr)
    , curr_corctx_(nullptr)
    , affix_(nullptr)
  {
  }

public:
  inline EvService& get_ev_service() noexcept
  {
    return evs_;
  }

  inline size_t get_index() const noexcept
  {
    return index_;
  }

  template <typename T>
  void set_affix(T* affix) noexcept
  {
    static_assert(std::is_base_of<affix_base, T>::value, "T must inhert from asev::affix_base");
    affix_ = affix;
  }

  template <typename T>
  T* get_affix()
  {
    static_assert(std::is_base_of<affix_base, T>::value, "T must inhert from asev::affix_base");
    return gsl::narrow<T*>(affix_);
  }

public:
  /// Internal use.
  template <typename Event, typename PoolMake = cque::pool_make<Event>>
  cque::mpsc_pool<Event, PoolMake>& get_event_pool(PoolMake pmk = PoolMake{})
  {
    std::type_index tyidx = typeid(Event);
    using event_pool_t = cque::mpsc_pool<Event, PoolMake>;
    for (auto& evp : pool_list_)
    {
      if (evp.tyidx_ == tyidx)
      {
        Expects(!!evp.pool_);
        return *gsl::narrow<event_pool_t*>(evp.pool_.get());
      }
    }

    event_pool evp(std::move(tyidx));
    evp.pool_.reset(new event_pool_t{pmk});
    pool_list_.push_back(std::move(evp));
    return *gsl::narrow<event_pool_t*>(pool_list_.back().pool_.get());
  }

  inline worker* get_worker() noexcept
  {
    return curr_worker_;
  }

  inline void set_worker(worker* wkr) noexcept
  {
    curr_worker_ = wkr;
  }

  corctx_t* get_corctx() noexcept
  {
    return curr_corctx_;
  }

  void set_corctx(corctx_t* corctx) noexcept
  {
    curr_corctx_ = corctx;
  }

private:
  EvService& evs_;
  size_t index_;
  worker* curr_worker_;
  std::list<event_pool> pool_list_;
  corctx_t* curr_corctx_;

  affix_base* affix_;
};
} /// namespace detail
} /// namespace asev

