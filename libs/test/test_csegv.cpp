///
/// Test csegv.
///

#include <actorx/utest/utest.hpp>

#include <actorx/csegv/all.hpp>

#include <thread>
#include <chrono>
#include <vector>
#include <list>
#include <iostream>


void badass()
{
  char* v = (char*)42;
  *v = 42;
}

void handler(std::list<csegv::stack_info> const& stack_list)
{
  std::cerr << "user segv handler" << std::endl;
  for (auto& ele : stack_list)
  {
    std::cerr << ele << std::endl;
  }
  std::cerr << std::endl << std::flush;
}

UTEST_CASE_FINAL(10000, test_csegv)
{
  csegv::init(3, true, handler);

  std::thread thr(
    []()
    {
      csegv::pcall(
        []()
        {
          int i = 0;
          badass();
          int b = i;
          ++b;
        });
    });
  thr.join();
  std::cout << "done." << std::endl;
}
