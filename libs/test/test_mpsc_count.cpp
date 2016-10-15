//
// Test mpsc_counter.
//

#include <actorx/utest/all.hpp>
#include <actorx/cque/all.hpp>

#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>


UTEST_CASE_SEQ(210, test_mpsc_count)
{
  try
  {
    auto const concurr = std::thread::hardware_concurrency();
    size_t const count = 100000;
    auto total = concurr * count;
    std::vector<std::thread> thread_pool;
    thread_pool.reserve(concurr);

    cque::mpsc_count<> ctr;
    using eclipse_clock_t = typename cque::mpsc_count<>::eclipse_clock_t;
    std::mutex mtx;
    std::condition_variable cv;

    auto bt = eclipse_clock_t::now();
    for (size_t i=0; i<concurr; ++i)
    {
      thread_pool.push_back(
        std::thread(
          [&ctr, &mtx, &cv, count]()
          {
            for (size_t i=0; i<count; ++i)
            {
              ctr.synchronized_incr(mtx, cv);
            }
          })
        );
    }

    while (true)
    {
      auto pr = ctr.synchronized_reset(mtx, cv, (eclipse_clock_t::duration::max)());
      total -= pr.first;
      if (total == 0)
      {
        break;
      }
    }

    for (auto& thr : thread_pool)
    {
      thr.join();
    }

    ACTX_ENSURES(total == 0)(total);

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
