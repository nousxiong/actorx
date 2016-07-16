//
// basic_thrctx.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"
#include "actorx/asev/affix_base.hpp"

#include <spdlog/spdlog.h>

#include <set>
#include <list>
#include <memory>
#include <typeinfo>
#include <typeindex>


namespace asev
{
namespace detail
{
// Forward declares.
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
  using logger_ptr = std::shared_ptr<spdlog::logger>;

public:
  basic_thrctx(EvService& evs, size_t index, logger_ptr logger)
    : evs_(evs)
    , index_(index)
    , logger_(logger)
    , curr_worker_(nullptr)
    , curr_corctx_(nullptr)
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

  inline logger_ptr get_logger() noexcept
  {
    return logger_;
  }

  template <typename T>
  T* add_affix(T* affix) noexcept
  {
    static_assert(std::is_base_of<affix_base, T>::value, "T must inhert from asev::affix_base");
    return affix_list_.add(affix);
  }

  template <typename T>
  T* get_affix() noexcept
  {
    static_assert(std::is_base_of<affix_base, T>::value, "T must inhert from asev::affix_base");
    return affix_list_.get<T>();
  }

  template <typename T>
  T* rmv_affix() noexcept
  {
    static_assert(std::is_base_of<affix_base, T>::value, "T must inhert from asev::affix_base");
    return affix_list_.rmv<T>();
  }

  std::vector<affix_base*> clear_affix() noexcept
  {
    return affix_list_.clear();
  }

public:
  //! Internal use.
  template <typename Event, typename PoolMake = cque::pool_make<Event>>
  cque::mpsc_pool<Event, PoolMake>& get_event_pool(PoolMake pmk = PoolMake{})
  {
    std::type_index tyidx = typeid(Event);
    using event_pool_t = cque::mpsc_pool<Event, PoolMake>;
    for (auto& evp : pool_list_)
    {
      if (evp.tyidx_ == tyidx)
      {
        ACTORX_EXPECTS(!!evp.pool_);
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
  logger_ptr logger_;

  worker* curr_worker_;
  std::list<event_pool> pool_list_;
  corctx_t* curr_corctx_;

  affix_list affix_list_;
};
} // namespace detail
} // namespace asev

