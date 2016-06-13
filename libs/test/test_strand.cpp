///
/// Test strand.
///

#include <actorx/utest/all.hpp>
#include <actorx/asev/all.hpp>

#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <type_traits>


UTEST_CASE(test_strand)
{
  struct my_event : public asev::event_base
  {
    void init(asev::ev_service& evs, size_t& c, size_t const total)
    {
      evs_ = &evs;
      c_ = &c;
      total_ = total;
    }

    bool handle(asev::thrctx_t& thrctx)
    {
      if (++(*c_) == total_)
      {
        evs_->stop();
      }
      return true;
    }

    asev::ev_service* evs_;
    size_t* c_;
    size_t total_;
  };

  asev::ev_service evs;
  auto const concurr = std::thread::hardware_concurrency() * 1;
  decltype(concurr) count = 100000;
  std::vector<std::thread> thread_pool;
  thread_pool.reserve(concurr);
  auto const total = concurr * count;

  size_t c = 0;
  asev::strand_t snd(evs);

  auto bt = std::chrono::high_resolution_clock::now();
  for (size_t n=0; n<concurr; ++n)
  {
    thread_pool.push_back(
      std::thread(
        [&snd, &c, count, total]()
        {
          auto& evs = snd.get_ev_service();
          for (size_t i=0; i<count; ++i)
          {
            auto ev = evs.make_event<my_event>();
            ev->init(evs, c, total);
            snd.async(ev);
          }
        })
      );
  }

  /// Start ev_service.
  evs.run();

  for (auto& thr : thread_pool)
  {
    thr.join();
  }

  auto et = std::chrono::high_resolution_clock::now();
  auto time_span =
    std::chrono::duration_cast<std::chrono::duration<double>>(et - bt);
  std::cout << "done, eclipse: " << time_span.count() << " s" << std::endl;
}
