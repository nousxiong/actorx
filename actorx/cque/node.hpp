//
// node.hpp
//

#pragma once

#include "actorx/cque/config.hpp"
#include "actorx/cque/node_base.hpp"

#include <memory>
#include <utility>


namespace cque
{
template <typename T, typename D = std::default_delete<node_base>>
class node : public node_base
{
  node(node&) = delete;
  node(node&&) = delete;
  node& operator=(node const&) = delete;
  node& operator=(node&&) = delete;

  using data_t = T;
  using deleter_t = D;
public:
  node() noexcept
  {
  }

  explicit node(data_t const& data) noexcept
    : data_(data)
  {
  }

  explicit node(data_t&& data) noexcept
    : data_(std::move(data))
  {
  }

public:
  void set_data(data_t const& data) noexcept
  {
    data_ = data;
  }

  void set_data(data_t&& data) noexcept
  {
    data_ = std::move(data);
  }

  data_t& get_data() noexcept
  {
    return data_;
  }

  data_t const& get_data() const noexcept
  {
    return data_;
  }

protected:
  void dispose() noexcept override
  {
    d_(this);
  }

private:
  data_t data_;
  deleter_t d_;
};

template <typename T, typename D = std::default_delete<node_base>>
struct node_maker
{
  node_base* operator()() const noexcept
  {
    return new node<T, D>();
  }
};
}
