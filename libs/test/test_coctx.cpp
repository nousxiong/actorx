///
/// Test coctx.
///

#include <actorx/utest/all.hpp>
#include <actorx/coctx/all.hpp>

#include <functional>
#include <iostream>


using namespace std::placeholders;

namespace usr
{
struct data
{
  explicit data(int& i)
    : i_(i)
  {
  }

  ~data()
  {
    ++i_;
    std::cout << "~data" << std::endl;
  }

  int& i_;
};

static int i = 1;

void handle(coctx::context& self, coctx::context& main_ctx)
{
  data d(i);
  std::cout << "OK, jump from main" << std::endl;
  self.jump(main_ctx);
  std::cout << "Back in handle" << std::endl;
}
}

UTEST_CASE(test_coctx)
{
  usr::i = 1;
  coctx::context main_ctx;

  auto ssize = coctx::make_stacksize();
  std::cout << "stack size: " << ssize.size() << std::endl;
  coctx::context ctx(
    [&main_ctx](coctx::context& self)
    {
      usr::handle(self, main_ctx);
      self.jump(main_ctx);
    },
    ssize
    );

  std::cout << "Create a ctx" << std::endl;
  main_ctx.jump(ctx);
  std::cout << "Back in main" << std::endl;
  main_ctx.jump(ctx);
  std::cout << "Back in main again" << std::endl;
  ACTORX_ENSURES(usr::i == 2)(usr::i);
  std::cout << "done." << std::endl;
}
