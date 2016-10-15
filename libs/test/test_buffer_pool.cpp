//
// Test buffer_pool.
//

#include <actorx/utest/all.hpp>

#include <actorx/all.hpp>

#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <memory>


UTEST_CASE_SEQ(300, test_buffer_pool)
{
  actx::detail::buffer_pool pool(0);

  auto const concurr = std::thread::hardware_concurrency();
  std::vector<std::thread> thrs;
  thrs.reserve(concurr);

  for (size_t i = 0; i < concurr; ++i)
  {
    thrs.push_back(
      std::thread(
        [&pool]()
        {
          auto buf1 = pool.get((1 << 7) - 1);
          ACTX_ENSURES(!!buf1);
          ACTX_ENSURES(buf1->use_count() == 1);
          ACTX_ENSURES(buf1->data().size() == 1 << 7)(buf1->data().size());
          auto buf2 = pool.get((1 << 9) - 1);
          ACTX_ENSURES(!!buf2);
          ACTX_ENSURES(buf2->use_count() == 1);
          ACTX_ENSURES(buf2->data().size() == 1 << 9)(buf2->data().size());

          auto buf1_addr = buf1.get();
          buf1.reset();
          ACTX_ENSURES(!buf1);

          auto buf3 = pool.get((1 << 7) - 1);
          ACTX_ENSURES(!!buf3);
          ACTX_ENSURES(buf3->data().size() == 1 << 7)(buf3->data().size());
          ACTX_ENSURES(buf1_addr == buf3.get());

          buf1 = buf3;
          ACTX_ENSURES(buf1->use_count() == 2);
          ACTX_ENSURES(buf3->use_count() == 2);
        })
      );
  }

  for (auto& thr : thrs)
  {
    thr.join();
  }

  std::cout << "done.\n";
}
