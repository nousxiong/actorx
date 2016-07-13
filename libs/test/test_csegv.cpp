//
// Test csegv.
//

#include <actorx/utest/all.hpp>
#include <actorx/asev/all.hpp>
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
        }, handler
        );
    });
  thr.join();
  std::cout << "done." << std::endl;
}

UTEST_CASE_FINAL(10001, test_asev_csegv)
{
  struct my_event : public asev::event_base
  {
    bool handle(asev::thrctx_t& thrctx)
    {
      auto i = 0;
      auto b = i;
      badass();
      ++b;
      return true;
    }
  };

  asev::ev_service evs;
  for (auto i = 0; i < 3; ++i)
  {
    auto ev = evs.make_event<my_event>();
    evs.async(ev);
  }
  // Start ev_service.
  evs.run();
  std::cout << "done." << std::endl;
}
