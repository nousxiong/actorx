//
// affix_base.hpp
//

#pragma once

#include "actorx/asev/config.hpp"

#include <vector>
#include <typeinfo>
#include <typeindex>


namespace asev
{
//! Affix of corctx or thrctx.
struct affix_base
{
  template <typename T>
  affix_base(T*)
    : tyidx_(typeid(T))
  {
  }

  virtual ~affix_base() {}

  std::type_index const tyidx_;
};

namespace detail
{
struct affix_list
{
  affix_list()
    : vec_size_(0)
    , vec_(ASEV_MAX_AFFIX_LIST_DIVISION, nullptr)
  {
  }

  //! Add an affix
  /**
    * @return Old(with same type) if any, or nullptr.
    */
  template <typename T>
  T* add(T* affix) noexcept
  {
    std::type_index tyidx = typeid(T);
    if (vec_size_ < vec_.size())
    {
      affix_base** first_empty_ptr = nullptr;
      size_t i = 0;
      for (auto& affix_ptr : vec_)
      {
        if (affix_ptr != nullptr)
        {
          if (affix_ptr->tyidx_ == tyidx)
          {
            auto old = affix_ptr;
            affix_ptr = affix;
            return gsl::narrow<T*>(old);
          }
          ++i;
        }

        if (first_empty_ptr == nullptr && affix_ptr == nullptr)
        {
          first_empty_ptr = &affix_ptr;
        }

        if (first_empty_ptr != nullptr && i >= vec_size_)
        {
          break;
        }
      }

      ACTX_ASSERTS(first_empty_ptr != nullptr);
      *first_empty_ptr = affix;
      ++vec_size_;
      return nullptr;
    }

    T* old = nullptr;
    auto pr = map_.emplace(std::move(tyidx), affix);
    if (!pr.second)
    {
      old = gsl::narrow<T*>(pr.first->second);
      pr.first->second = affix;
    }
    return old;
  }

  //! Get given type affix.
  /**
  * @return Get affix if any, or nullptr.
  */
  template <typename T>
  T* get() noexcept
  {
    std::type_index const tyidx = typeid(T);
    // First find in vec_.
    for (size_t i = 0; i < vec_size_; )
    {
      auto affix_ptr = vec_[i];
      if (affix_ptr != nullptr)
      {
        if (affix_ptr->tyidx_ == tyidx)
        {
          return gsl::narrow<T*>(affix_ptr);
        }
        ++i;
      }
    }

    auto itr = map_.find(tyidx);
    if (itr != map_.end())
    {
      return gsl::narrow<T*>(itr->second);
    }
    return nullptr;
  }

  //! Remove an affix with type T.
  /**
    * @return Removed affix if any, or nullptr.
    */
  template <typename T>
  T* rmv() noexcept
  {
    std::type_index const tyidx = typeid(T);
    // First find in vec_.
    for (size_t i = 0; i < vec_size_; )
    {
      auto& affix_ptr = vec_[i];
      if (affix_ptr != nullptr)
      {
        if (affix_ptr->tyidx_ == tyidx)
        {
          auto old = affix_ptr;
          affix_ptr = nullptr;
          --vec_size_;
          return gsl::narrow<T*>(old);
        }
        ++i;
      }
    }

    T* old = nullptr;
    auto itr = map_.find(tyidx);
    if (itr != map_.end())
    {
      old = gsl::narrow<T*>(itr->second);
      map_.erase(itr);
    }
    return old;
  }

  //! Clear affixes.
  /**
  * @return All affixes.
  */
  std::vector<affix_base*> clear()
  {
    std::vector<affix_base*> affixes;
    affixes.reserve(vec_size_ + map_.size());

    for (auto& affix_ptr : vec_)
    {
      if (affix_ptr != nullptr)
      {
        affixes.push_back(affix_ptr);
        affix_ptr = nullptr;
      }
    }
    vec_size_ = 0;

    for (auto& pr : map_)
    {
      affixes.push_back(pr.second);
    }
    map_.clear();

    return std::move(affixes);
  }

  size_t vec_size_;
  std::vector<affix_base*> vec_;
  std::map<std::type_index, affix_base*> map_;
};
} // namespace detail
} // namespace asev
