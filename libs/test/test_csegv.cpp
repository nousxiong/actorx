///
/// Test csegv.
///

#include <actorx/utest/utest.hpp>

#include <actorx/csegv/all.hpp>

#include <thread>
#include <chrono>
#include <vector>
#include <iostream>


#ifdef ACTORX_TEST_CSEGV

void badass()
{
  char* v = (char*)42;
  *v = 42;
}

UTEST_CASE_FINAL(10000, test_csegv)
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

#endif
