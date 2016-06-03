///
/// stack.hpp
///

#pragma once

#include <csegv/config.hpp>

#include <string>
#include <iostream>
#include <cassert>


namespace csegv
{
namespace detail
{
static size_t max_stack_depth(size_t size = 0)
{
  static size_t depth(3);
  if (size > 0)
  {
    depth = size;
  }
  return depth;
}

static bool is_brief_info(int brief = 0)
{
  static bool is_brief = true;
  if (brief != 0)
  {
    is_brief = brief < 0;
  }
  return is_brief;
}

struct stack_info
{
  stack_info()
    : line_(-1)
  {
  }

  stack_info(stack_info&& s)
    : line_(s.line_)
    , file_(std::move(s.file_))
    , module_(std::move(s.module_))
    , symbol_(std::move(s.symbol_))
  {
  }

  stack_info(stack_info const& s)
    : line_(s.line_)
    , file_(s.file_)
    , module_(s.module_)
    , symbol_(s.symbol_)
  {
  }

  void operator=(stack_info&& s)
  {
    line_ = s.line_;
    file_ = std::move(s.file_);
    module_ = std::move(s.module_);
    symbol_ = std::move(s.symbol_);
  }

  void operator=(stack_info const& s)
  {
    line_ = s.line_;
    file_ = s.file_;
    module_ = s.module_;
    symbol_ = s.symbol_;
  }

  friend std::ostream& operator <<(std::ostream& out, stack_info const& s)
  {
    auto brief = is_brief_info(); 
    auto file = s.file_;
    auto module = s.module_;
    if (brief)
    {
      auto file_found = file.find_last_of("/\\");
      file = file.substr(file_found + 1);
      auto module_found = module.find_last_of("/\\");
      module = module.substr(module_found + 1);
    }
    out << "\nfile: " << file << " - line " << s.line_ << "\nmodule: " << module << "\nsymbol: " << s.symbol_;
    return out;
  }

  int line_;
  std::string file_;
  std::string module_;
  std::string symbol_;
};
} /// namespace detail
} /// namespace csegv
