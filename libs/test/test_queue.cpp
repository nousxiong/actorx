///
/// Test queue.
///

#include <actorx/utest/all.hpp>
#include <actorx/cque/all.hpp>

#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <type_traits>


UTEST_CASE_SEQ(200, test_queue)
{
  struct data : public cque::node_base
  {
    size_t i_;
    std::string str_;
  };

  try
  {
    size_t count = 100000;
    cque::mpsc_pool<data> pool;
    cque::queue<data> que;

    // Pre alloc nodes
    for (size_t i=0; i<count; ++i)
    {
      pool.free(new data);
    }
    auto bt = std::chrono::high_resolution_clock::now();

    for (size_t i=0; i<count; ++i)
    {
      auto e = cque::get<data>(pool);
      e->i_ = i;
      e->str_ = "test string";
      que.push(e);
    }

    for (size_t i=0; i<count; ++i)
    {
      auto e = que.pop_unique<cque::pool_delete<data>>();
      Ensures(e->i_ == i);
      Ensures(e->str_ == "test string");
    }

    auto et = std::chrono::high_resolution_clock::now();
    auto time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(et - bt);
    std::cout << "done, eclipse: " << time_span.count() << " s" << std::endl;
  }
  catch (std::exception& ex)
  {
    std::cerr << "except: " << ex.what() << std::endl;
  }
  std::cout << "quit." << std::endl;
}
