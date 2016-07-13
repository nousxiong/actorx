//
// stack_info.hpp
//

#pragma once

#include "actorx/csegv/config.hpp"

#include <string>
#include <list>
#include <iostream>
#include <functional>
#include <cassert>


namespace csegv
{
// Forward declare.
struct stack_info;

// Segv's handler.
using handler_t = std::function<void (std::list<stack_info> const&)>;

//! Stack info.
struct stack_info
{
  stack_info(bool brief = true)
    : brief_(brief)
    , line_(-1)
  {
  }

  stack_info(stack_info&& s)
    : brief_(s.brief_)
    , line_(s.line_)
    , file_(std::move(s.file_))
    , module_(std::move(s.module_))
    , symbol_(std::move(s.symbol_))
  {
  }

  stack_info(stack_info const& s)
    : brief_(s.brief_)
    , line_(s.line_)
    , file_(s.file_)
    , module_(s.module_)
    , symbol_(s.symbol_)
  {
  }

  void operator=(stack_info&& s)
  {
    brief_ = s.brief_;
    line_ = s.line_;
    file_ = std::move(s.file_);
    module_ = std::move(s.module_);
    symbol_ = std::move(s.symbol_);
  }

  void operator=(stack_info const& s)
  {
    brief_ = s.brief_;
    line_ = s.line_;
    file_ = s.file_;
    module_ = s.module_;
    symbol_ = s.symbol_;
  }

  friend std::ostream& operator<<(std::ostream& out, stack_info const& s)
  {
    auto file = s.file_;
    auto module = s.module_;
    if (s.brief_)
    {
      auto file_found = file.find_last_of("/\\");
      file = file.substr(file_found + 1);
      auto module_found = module.find_last_of("/\\");
      module = module.substr(module_found + 1);
    }
    out << "\nfile: " << file << " - line " << s.line_ << "\nmodule: " << module << "\nsymbol: " << s.symbol_;
    return out;
  }

  bool brief_;
  int line_;
  std::string file_;
  std::string module_;
  std::string symbol_;
};
} // namespace csegv
