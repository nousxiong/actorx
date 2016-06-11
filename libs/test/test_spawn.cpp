///
/// Test spawn.
///

#include <actorx/utest/all.hpp>

#ifdef _WIN32
# ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0700
# endif
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/system_timer.hpp>

#include <actorx/asev/all.hpp>

#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <memory>


namespace usr
{
struct timer
{
  timer(asio::io_service& ios, asev::corctx_t& self)
    : tmr_(ios)
    , self_(self)
  {
  }

  template <class Duration>
  void sleep_for(Duration dur)
  {
    tmr_.expires_from_now(std::forward<Duration>(dur));
    tmr_.async_wait(
      [this](asio::error_code)
      {
        self_.resume();
      });
    self_.yield();
  }

  asio::system_timer tmr_;
  asev::corctx_t& self_;
};
}


UTEST_CASE(test_spawn)
{
  asio::io_service ios;
  std::unique_ptr<asio::io_service::work> work(new asio::io_service::work(ios));

  asev::ev_service evs;
  asev::strand_t snd(evs);
  size_t const total = 200;
  std::atomic_size_t count(total);

  for (size_t i = 0; i < total; ++i)
  {
    evs.spawn(
      [&](asev::corctx_t& self)
      {
        usr::timer tmr(ios, self);

        for (size_t i=0; i<5; ++i)
        {
          auto bt = std::chrono::system_clock::now();
          tmr.sleep_for(std::chrono::milliseconds(100));
          auto eclipse =
            std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now() - bt
              );
          Ensures(eclipse >= std::chrono::milliseconds(100));
        }

        if (--count == 0)
        {
          evs.stop();
        }
      });
  }

  std::thread thr{[&ios](){ios.run();}};
  evs.run();
  work.reset();
  thr.join();

  std::cout << "done." << std::endl;

#ifdef SPDLOG_DEBUG_ON
  Ensures(false);
#endif // SPDLOG_DEBUG_ON
}
