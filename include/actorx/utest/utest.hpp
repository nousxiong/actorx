///
/// utest.hpp
///

#pragma once

#include "actorx/user.hpp"
#include "actorx/csegv/all.hpp"

#include <gsl.h>

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

  void run(int loop = 1, std::chrono::milliseconds interval = std::chrono::milliseconds(100))
  {
    /// Init csegv.
    csegv::init();

    /// Begin utest loop.
    csegv::pcall(
      [this, loop, interval]()
      {
        for (total_loop_=0; total_loop_<loop; ++total_loop_)
        {
          for (auto const& cpr : seq_case_list_)
          {
            if (run_case(cpr.second))
            {
              std::cerr << "\n------------------------------------------" << std::endl;
              std::cerr << "seq case " << cpr.second.name_ << " not pass, others will not be executed!" << std::endl;
              std::cerr << "------------------------------------------" << std::endl;
              goto test_end;
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

          std::this_thread::sleep_for(interval);
        }

test_end:
        std::cout << std::endl << "Total utest loop: " << total_loop_ << std::endl;
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

struct auto_unreg
{
  explicit auto_unreg(std::string name)
  {
    auto ctx = utest::context::instance();
    ctx->add_disabled(std::move(name));
  }
};
}

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

#define UTEST_CASE_SEQ(priority, case_name) UTEST_CASE_SEQ_IMPL(priority, case_name, false)
#define UTEST_CASE_FINAL(priority, case_name) UTEST_CASE_SEQ_IMPL(priority, case_name, true)

#define UTEST_DISABLE(case_name) \
  namespace \
  { \
    utest::auto_unreg utest_##case_name##_aur(#case_name); \
  }

#define UTEST_MAIN_LOOP(loop, interval) \
  int main() \
  { \
    auto ctx = utest::context::instance(); \
    auto iv = std::chrono::milliseconds(interval); \
    ctx->run(loop, iv); \
    return 0; \
  }

#define UTEST_MAIN UTEST_MAIN_LOOP(1, 0)
