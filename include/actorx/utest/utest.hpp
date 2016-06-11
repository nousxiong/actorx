///
/// utest.hpp
///

#pragma once

#include "actorx/utest/config.hpp"

#include <thread>
#include <chrono>
#include <map>
#include <set>
#include <list>
#include <string>
#include <iostream>
#include <functional>


namespace utest
{
struct ucase
{
  std::string name_;
  std::function<void ()> f_;
};

class context
{
public:
  static context* instance()
  {
    static context ctx;
    return &ctx;
  }

  template <typename F>
  void add_case(std::string name, F f)
  {
    ucase uc;
    uc.name_ = std::move(name);
    uc.f_ = std::move(f);
    case_list_.push_back(std::move(uc));
  }

  template <typename F>
  void add_case(int priority, std::string name, F f, bool is_final)
  {
    pri_add_case(
      is_final ? final_case_list_ : seq_case_list_,
      priority, std::move(name), std::forward<F>(f)
      );
  }

  void add_disabled(std::string name)
  {
    disabled_list_.insert(std::move(name));
  }

  void add_enabled(std::string name)
  {
    enabled_list_.insert(std::move(name));
  }

  void run()
  {
    /// Init csegv.
    csegv::init();

    if (!enabled_list_.empty())
    {
      auto seq_case_list = seq_case_list_;
      seq_case_list_.clear();
      for (auto& cpr : seq_case_list)
      {
        if (enabled_list_.find(cpr.second.name_) != enabled_list_.end())
        {
          seq_case_list_.emplace(cpr.first, std::move(cpr.second));
        }
      }

      auto case_list = case_list_;
      case_list_.clear();
      for (auto& c : case_list_)
      {
        if (enabled_list_.find(c.name_) != enabled_list_.end())
        {
          case_list_.push_back(std::move(c));
        }
      }

      auto final_case_list = final_case_list_;
      final_case_list_.clear();
      for (auto& cpr : final_case_list)
      {
        if (enabled_list_.find(cpr.second.name_) != enabled_list_.end())
        {
          final_case_list_.emplace(cpr.first, std::move(cpr.second));
        }
      }
    }

    /// Begin utest loop.
    csegv::pcall(
      [this]()
      {
        for (auto const& cpr : seq_case_list_)
        {
          if (run_case(cpr.second))
          {
            std::cerr << "\n------------------------------------------" << std::endl;
            std::cerr << "seq case " << cpr.second.name_ << " not pass, others will not be executed!" << std::endl;
            std::cerr << "------------------------------------------" << std::endl;
            return;
          }
        }

        for (auto const& c : case_list_)
        {
          run_case(c);
        }

        for (auto const& cpr : final_case_list_)
        {
          run_case(cpr.second);
        }
      });
  }

private:
  bool run_case(ucase const& c) noexcept
  {
    bool except = false;
    if (disabled_list_.find(c.name_) != disabled_list_.end())
    {
      return except;
    }

    try
    {
      std::cout << "--------------<" << c.name_ << "> begin--------------" << std::endl;
      c.f_();
      std::cout << "--------------<" << c.name_ << "> end----------------" << std::endl;
    }
    catch (std::exception& ex)
    {
      std::cout << "!!!!!!!!!!!!!--<" << c.name_ << "> except: " << ex.what() << std::endl;
      except = true;
    }
    std::cout << std::endl;
    return except;
  }

  template <typename F>
  static void pri_add_case(
    std::map<int, ucase>& case_list,
    int priority, std::string name, F f
    )
  {
    ucase uc;
    uc.name_ = name;
    uc.f_ = std::move(f);
    auto pr = case_list.emplace(priority, std::move(uc));
    if (!pr.second)
    {
      std::cerr << "case " << name << " with priority: " <<
        priority << ", repeat with case " << pr.first->second.name_ << std::endl;
      std::terminate();
    }
  }

private:
  std::map<int, ucase> seq_case_list_;
  std::list<ucase> case_list_;
  std::map<int, ucase> final_case_list_;
  std::set<std::string> disabled_list_;
  std::set<std::string> enabled_list_;
  int total_loop_;
};

struct auto_reg
{
  template <typename F>
  auto_reg(std::string name, F f)
  {
    auto ctx = utest::context::instance();
    ctx->add_case(std::move(name), std::forward<F>(f));
  }

  template <typename F>
  auto_reg(int priority, std::string name, F f, bool is_final)
  {
    auto ctx = utest::context::instance();
    ctx->add_case(priority, std::move(name), std::forward<F>(f), is_final);
  }
};

struct auto_disable
{
  explicit auto_disable(std::string name)
  {
    auto ctx = utest::context::instance();
    ctx->add_disabled(std::move(name));
  }
};

struct auto_enable
{
  explicit auto_enable(std::string name)
  {
    auto ctx = utest::context::instance();
    ctx->add_enabled(std::move(name));
  }
};
} /// namespace utest

/// Add an test case named by case_name.
#define UTEST_CASE(case_name) \
  static void utest_##case_name##_f(); \
  namespace \
  { \
    utest::auto_reg utest_##case_name##_ar(#case_name, &utest_##case_name##_f); \
  } \
  static void utest_##case_name##_f()

#define UTEST_CASE_SEQ_IMPL(priority, case_name, is_final) \
  static void utest_##case_name##_f(); \
  namespace \
  { \
    utest::auto_reg utest_##case_name##_ar(priority, #case_name, &utest_##case_name##_f, is_final); \
  } \
  static void utest_##case_name##_f()

/// Add an test case with priority, those cases prior all the others.
#define UTEST_CASE_SEQ(priority, case_name) UTEST_CASE_SEQ_IMPL(priority, case_name, false)

/// Add an test case will be run after all the others.
#define UTEST_CASE_FINAL(priority, case_name) UTEST_CASE_SEQ_IMPL(priority, case_name, true)

/// Disable given test case.
#define UTEST_DISABLE(case_name) \
  namespace \
  { \
    utest::auto_disable utest_##case_name##_ad(#case_name); \
  }

/// When use this macro, all case will be disabled and only this macro's case(s)
/// BUT not in disabled_list can run.
#define UTEST_ENABLE(case_name) \
  namespace \
  { \
    utest::auto_enable utest_##case_name##_ae(#case_name); \
  }

/// User must use this macro in one and only one cpp file.
#define UTEST_MAIN \
  int main() \
  { \
    auto ctx = utest::context::instance(); \
    ctx->run(); \
    return 0; \
  }
