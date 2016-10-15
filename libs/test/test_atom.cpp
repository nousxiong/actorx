//
// Test atom.
//

#include <actorx/utest/all.hpp>

#include <actorx/all.hpp>

#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <memory>


UTEST_CASE(test_atom)
{
  char const s1[] = "zzzzzzzzzz";
  char const s2[] = "INIT";
  char const s3[] = "0";
  auto a1 = actx::atom(s1);
  auto a2 = actx::atom(s2);
  auto a3 = actx::atom(s3);

  std::cout << "[" << s1 << "] -> " << a1 << std::endl;
  std::cout << "[" << s1 << "] <- " << a1 << std::endl;
  std::cout << "[" << s2 << "] -> " << a2 << std::endl;
  std::cout << "[" << s2 << "] <- " << a2 << std::endl;
  std::cout << "[" << s3 << "] -> " << a3 << std::endl;
  std::cout << "[" << s3 << "] <- " << a3 << std::endl;

  ACTX_ENSURES(actx::dynatom(s1) == a1);
  ACTX_ENSURES(actx::dynatom(s2) == a2);
  ACTX_ENSURES(actx::dynatom(s3) == a3);

  auto i = actx::atom("INIT");
  switch (i)
  {
  case actx::atom("INIT"): break;
  default: ACTX_ENSURES(false);
  }
}
