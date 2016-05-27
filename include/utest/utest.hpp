///
/// utest.hpp
///

#pragma once

#include <gsl.h>
#include <thread>
#include <map>
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
  void add_case(int priority, std::string name, F f)
  {
    ucase uc;
    uc.name_ = name;
    uc.f_ = std::move(f);
    auto pr = seq_case_list_.emplace(priority, std::move(uc));
    if (!pr.second)
    {
      std::cerr << "case " << name << " with priority: " <<
        priority << ", repeat with case " << pr.first->second.name_ << std::endl;
      std::terminate();
    }
  }

  void run()
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
  }

private:
  static bool run_case(ucase const& c)
  {
    bool except = false;
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

private:
  std::map<int, ucase> seq_case_list_;
  std::list<ucase> case_list_;
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
  auto_reg(int priority, std::string name, F f)
  {
    auto ctx = utest::context::instance();
    ctx->add_case(priority, std::move(name), std::forward<F>(f));
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

#define UTEST_CASE_SEQ(priority, case_name) \
  static void utest_##case_name##_f(); \
  namespace \
  { \
    utest::auto_reg utest_##case_name##_ar(priority, #case_name, &utest_##case_name##_f); \
  } \
  static void utest_##case_name##_f()


#define UTEST_MAIN \
  int main() \
  { \
    auto ctx = utest::context::instance(); \
    ctx->run(); \
    return 0; \
  }
