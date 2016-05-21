///
/// Test eout.
///

#include <utest/utest.hpp>

#include <asev/all.hpp>

#include <thread>
#include <chrono>
#include <vector>
#include <iostream>


UTEST_CASE_SEQ(1000, test_eout)
{
  asev::eout("Hello, {}!\n", "world");

  /// Test multi-threads output.
  size_t const concurr = std::thread::hardware_concurrency();
  std::vector<std::thread> thread_pool;
  thread_pool.reserve(concurr);

  auto bt = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < concurr; ++i)
  {
    thread_pool.push_back(
      std::thread(
        []()
        {
          for (size_t i = 0; i < 1000; ++i)
          {
            asev::eout("\rThe answer is {}, but the question unknown!, do {} some works!", 42, 52232.3f);
          }
        })
      );
  }

  for (auto& thr : thread_pool)
  {
    thr.join();
  }

  auto et = std::chrono::high_resolution_clock::now();
  auto time_span =
    std::chrono::duration_cast<std::chrono::duration<double>>(et - bt);
  asev::eout("\ndone, eclipse: {} s\n", time_span.count());

  asev::eout("Goodbye, {}!\n", "world");
}
