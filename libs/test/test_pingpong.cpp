///
/// Test pingpong.
///

#include <actorx/utest/utest.hpp>

#include <actorx/asev/all.hpp>

#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <type_traits>
#include <cassert>


UTEST_CASE(test_pingpong)
{
  using ut_clock_t = std::chrono::high_resolution_clock;
  using ut_time_t = ut_clock_t::time_point;

  struct pingpong_event : public asev::event_base
  {
    struct make
    {
      explicit make(std::vector<asev::strand_t>& snds)
        : snds_(snds)
      {
      }

      gsl::owner<pingpong_event*> operator()() const noexcept
      {
        return new (std::nothrow) pingpong_event(snds_);
      }

      std::vector<asev::strand_t>& snds_;
    };

    explicit pingpong_event(std::vector<asev::strand_t>& snds)
      : snds_(snds)
      , curr_(0)
      , c_(0)
      , ec_(nullptr)
      , total_(0)
      , utcnt_(nullptr)
    {
    }

    void init(
      size_t const total, size_t const batch, std::atomic_size_t* ec,
      size_t* utcnt, ut_time_t bt
    )
    {
      curr_ = 0;
      total_ = total;
      c_ = 0;
      batch_ = batch;
      ec_ = ec;
      utcnt_ = utcnt;
      bt_ = bt;
    }

    bool handle(asev::thrctx_t& thrctx)
    {
      bool is_auto = false;
      auto c = c_++;
      if (c == total_)
      {
        is_auto = true;
        if (++(*ec_) == batch_)
        {
          std::cout << "test finished, remaining: " << *utcnt_ << std::endl;
          auto et = ut_clock_t::now();
          auto time_span =
            std::chrono::duration_cast<std::chrono::duration<double>>(et - bt_);
          std::cout << "  eclipse: " << time_span.count() << " s" << std::endl;

          *ec_ = 0;
          if (--(*utcnt_) == 0)
          {
            auto& evs = thrctx.get_ev_service();
            evs.stop();
          }
          else
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            /// Start one new ut.
            snds_.front().post(
              [this](asev::thrctx_t& thrctx)
              {
                auto& evs = thrctx.get_ev_service();
                auto& snd = snds_.front();
                pingpong_event::make ppev_make(snds_);
                auto bt = ut_clock_t::now();
                for (size_t i = 0; i < batch_; ++i)
                {
                  auto ev = evs.make_event<pingpong_event>(ppev_make);
                  ev->init(total_, batch_, ec_, utcnt_, bt);
                  snd.async(ev);
                }
              });
          }
        }
      }
      else
      {
        ++curr_;
        if (curr_ >= snds_.size())
        {
          curr_ = 0;
        }
        snds_[curr_].async(this);
      }
      return is_auto;
    }

    std::vector<asev::strand_t>& snds_;
    size_t curr_;
    size_t c_;
    size_t batch_;
    std::atomic_size_t* ec_;
    size_t total_;

    size_t* utcnt_;
    ut_time_t bt_;
  };

  auto const concurr = std::thread::hardware_concurrency()/* / 2*/;
  decltype(concurr) count = 100000;
  auto const total = concurr * count;

  asev::ev_service evs(concurr);
  std::vector<asev::strand_t> snds;
  for (size_t i = 0; i < concurr; ++i)
  {
    snds.emplace_back(std::ref(evs));
  }
  std::atomic_size_t ec(0);
  size_t utcnt = 1;

  snds.front().post(
    [&snds, total, &ec, &utcnt](asev::thrctx_t& thrctx)
    {
      auto& evs = thrctx.get_ev_service();
      size_t batch = 100;
      auto turn = total / batch;
      auto& snd = snds.front();
      pingpong_event::make ppev_make(snds);
      auto bt = ut_clock_t::now();
      for (size_t i = 0; i < batch; ++i)
      {
        auto ev = evs.make_event<pingpong_event>(ppev_make);
        ev->init(turn, batch, &ec, &utcnt, bt);
        snd.async(ev);
      }
    });

  auto bt = ut_clock_t::now();
  /// Start ev_service.
  evs.run();
  auto et = ut_clock_t::now();

  auto time_span =
    std::chrono::duration_cast<std::chrono::duration<double>>(et - bt);
  std::cout << "done, eclipse: " << time_span.count() << " s" << std::endl;

#ifdef SPDLOG_DEBUG_ON
  Ensures(false);
#endif // SPDLOG_DEBUG_ON
}
