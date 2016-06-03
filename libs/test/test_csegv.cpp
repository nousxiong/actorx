///
/// Test csegv.
///

#include <utest/utest.hpp>

#include <csegv/all.hpp>

#include <thread>
#include <chrono>
#include <vector>
#include <iostream>


void badass()
{
  char* v = (char*)42;
  *v = 42;
}

UTEST_CASE_SEQ(50, test_csegv)
{
  csegv::init();

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
