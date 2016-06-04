///
/// ev_service.hpp
///

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/eout.hpp"
#include "actorx/asev/detail/worker.hpp"
#include "actorx/asev/detail/basic_thrctx.hpp"
#include "actorx/asev/detail/basic_corctx.hpp"
#include "actorx/asev/detail/basic_strand.hpp"
#include "actorx/asev/detail/post_event.hpp"
#include "actorx/asev/detail/spawn_event.hpp"
#include "actorx/asev/detail/tstart_event.hpp"
#include "actorx/asev/detail/texit_event.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <deque>
#include <atomic>
#include <memory>
#include <functional>
#include <typeinfo>
#include <typeindex>


namespace asev
{
/// Provides async event functionality.
class ev_service
{
  using strand_t = detail::basic_strand<ev_service>;
  using thrctx_t = detail::basic_thrctx<ev_service>;
  using corctx_t = detail::basic_corctx<ev_service>;
  using post_handler_t = std::function<void (thrctx_t&)>;
  using coro_handler_t = std::function<void (corctx_t&)>;
  using tstart_handler_t = std::function<void (thrctx_t&)>;
  using texit_handler_t = std::function<void (thrctx_t&)>;
  using uid_t = unsigned int;
  using worker_ptr = std::atomic<detail::worker*>;

  friend class detail::basic_strand<ev_service>;
  friend class detail::basic_corctx<ev_service>;

#ifdef ASEV_SYSTEM_CLOCK
  using eclipse_clock_t = std::chrono::system_clock;
#else
  using eclipse_clock_t = std::chrono::steady_clock;
#endif // ASEV_SYSTEM_CLOCK

  ev_service(ev_service const&) = delete;
  ev_service& operator=(ev_service const&) = delete;
  ev_service(ev_service&&) = delete;
  ev_service& operator=(ev_service&&) = delete;

public:
  explicit ev_service(
    size_t thread_num = std::thread::hardware_concurrency(),
    size_t worker_num = 0
    )
    : uid_(0)
    , curr_sndidx_(0)
    , stopped_workers_(0)
  {
    static std::atomic_uint evs_uid(0);
    uid_ = evs_uid++;

    thread_num = thread_num == 0 ? 1 : thread_num;

    /// Worker num must >= thread_num
    if (worker_num < thread_num)
    {
      worker_num = thread_num;
    }

    /// Make thread data.
    for (size_t i=0; i<thread_num; ++i)
    {
      thread_data_list_.emplace_back(std::ref(*this), i);
    }

    /// Make workers.
    for (size_t i=0; i<worker_num; ++i)
    {
      worker_list_.emplace_back(worker_num, i);
    }

    /// Push all workers into workshop_.
    for (auto& wkr : worker_list_)
    {
      workshop_.emplace_back(&wkr);
    }
  }

  ~ev_service()
  {
    /// Clean up all misc.
    for (auto& thrdat : thread_data_list_)
    {
      while (true)
      {
        auto ev = thrdat.tstart_que_.pop_unique<cque::pool_delete<threv_base>>();
        if (!ev)
        {
          break;
        }
      }

      while (true)
      {
        auto ev = thrdat.texit_que_.pop_unique<cque::pool_delete<threv_base>>();
        if (!ev)
        {
          break;
        }
      }
    }

    workshop_.clear();
    worker_list_.clear();

    while (true)
    {
      auto pool = local_pool_queue_.pop_unique();
      if (!pool)
      {
        break;
      }
    }

    int64_t tworks = 0;
    int64_t pworks = 0;
    int64_t mworks = 0;
    for (auto& thrdat : thread_data_list_)
    {
      pworks += thrdat.pworks_;
      mworks += thrdat.mworks_;
      tworks += thrdat.pworks_ + thrdat.mworks_;
    }
    /// @todo only for test.
    eout("p: {}, m: {}, t: {}\n", pworks, mworks, tworks);
  }

public:
  /// Get thread num.
  inline size_t get_thread_num() const noexcept
  {
    return thread_data_list_.size();
  }

  /// Get worker num.
  inline size_t get_worker_num() const noexcept
  {
    return worker_list_.size();
  }

  /// Post a handler into background thread pool to run.
  /**
   * @param f Handler that will be posted.
   */
  template <typename F>
  void post(F&& f)
  {
    pri_post(select_strand_index(), post_handler_t(f));
  }

  /// Spawn an coroutine into background thread pool.
  template <typename F>
  void spawn(F&& f, coctx::stack_size ssize = coctx::make_stacksize())
  {
    pri_spawn(select_strand_index(), coro_handler_t(f), ssize);
  }

  /// Add an user defined event into background thread pool to run.
  /**
   * @param ev User defined event.
   */
  void async(gsl::owner<event_base*> ev)
  {
    pri_async(select_strand_index(), ev);
  }

  /// Post a handler into all background threads to run when their start.
  /**
   * @param f Handler that will run when each background thread start.
   */
  template <typename F>
  void tstart(F&& f)
  {
    pri_tstart(tstart_handler_t(f));
  }

  /// Post a handler into all background threads to run when their exit.
  /**
   * @param f Handler that will run when each background thread exit.
   */
  template <typename F>
  void texit(F&& f)
  {
    pri_texit(texit_handler_t(f));
  }

  /// Make an event using thread_local pool.
  template <typename Event, typename PoolMake = cque::pool_make<Event>>
  gsl::owner<Event*> make_event(PoolMake pmk = PoolMake{})
  {
    using event_pool_t = cque::mpsc_pool<Event, PoolMake>;

    auto thrctx = current();
    event_pool_t* pool = nullptr;
    if (thrctx == nullptr)
    {
      /// Non-trivial object with thread_local storage hasn't been implemented in gcc yet.
      static thread_local thread_local_pool_array pool_array{0};
      if (pool_array.size_ == 0)
      {
        for (auto& ptr : pool_array.arr_)
        {
          ptr = nullptr;
        }
      }

      Expects(uid_ < ASEV_MAX_EV_SERVICE);

      auto& curr_pool = pool_array.arr_[uid_];
      if (curr_pool == nullptr)
      {
        auto ptr = new thread_local_pool;
        ptr->pool_.reset(new event_pool_t(pmk));
        local_pool_queue_.push(ptr);
        curr_pool = ptr;
        ++pool_array.size_;
      }
      pool = gsl::narrow<event_pool_t*>(curr_pool->pool_.get());
    }
    else
    {
      pool = &thrctx->get_event_pool<Event, PoolMake>(pmk);
    }

    Ensures(pool != nullptr);
    return cque::get<Event>(*pool);
  }

  /// Start ev_service to run and block current thread to wait for stop.
  void run()
  {
    /// Start thread_pool.
    std::vector<std::thread> thread_pool;
    auto thread_num = thread_data_list_.size();
    thread_pool.reserve(thread_num);
    for (size_t i=0; i<thread_num; ++i)
    {
      thread_pool.push_back(
        std::thread(
          [this, i]()
          {
            auto& thrdat = thread_data_list_[i];
            auto& thrctx = *pri_current(thrdat.thrctx_.get());
            auto const thread_num = thread_data_list_.size();
            auto const worker_num = worker_list_.size();

            /// Get prior, minor and inferior workers.
            std::vector<size_t> priors;
            std::vector<size_t> minors;
            for (size_t n=0; n<worker_num; ++n)
            {
              auto mod = n % thread_num;
              if (mod == i)
              {
                priors.push_back(n);
              }
              else
              {
                minors.push_back(n);
              }
            }

            /// Run all tstart event.
            while (true)
            {
              auto ev = thrdat.tstart_que_.pop();
              if (ev == nullptr)
              {
                break;
              }

              bool is_auto = true;
              try
              {
                is_auto = ev->handle(thrctx);
              }
              catch (...)
              {
                Ensures(false);
              }

              if (is_auto)
              {
                ev->release();
              }
            }

            /// Set callback to run all texit.
            auto texit = gsl::finally(
              [&thrdat, &thrctx]()
              {
                while (true)
                {
                  auto ev = thrdat.texit_que_.pop();
                  if (ev == nullptr)
                  {
                    break;
                  }

                  bool is_auto = true;
                  try
                  {
                    is_auto = ev->handle(thrctx);
                  }
                  catch (...)
                  {
                    Ensures(false);
                  }

                  if (is_auto)
                  {
                    ev->release();
                  }
                }
              });

            /// Poll loop sleep duration.
            std::chrono::microseconds const poll_sleep_dur{50};

            /// Work loop.
            while (!thrdat.is_stop())
            {
              try
              {
                /// First try aggressive polling.
                for (size_t i=0; i<100; ++i)
                {
                  if (thrdat.cnt_.reset() > 0)
                  {
                    goto do_job;
                  }
                }

                /// Then moderate polling.
                for (size_t i=0; i<500; ++i)
                {
                  if (thrdat.cnt_.reset() > 0 || thrdat.is_stop())
                  {
                    goto do_job;
                  }
                  std::this_thread::sleep_for(poll_sleep_dur);
                }

                /// Finally waiting notify.
                thrdat.cnt_.synchronized_reset(thrdat.mtx_, thrdat.cv_);

              do_job:
                if (thrdat.is_stop())
                {
                  break;
                }
              }
              catch (...)
              {
                Ensures(false);
              }

              /// Firstly try run prior workers.
              size_t pworks = 0;
              for (auto n : priors)
              {
                /// Try pop a worker to run.
                pworks += do_work(n, thrctx);
              }

              /// @todo works dynamic load balance.
              if (pworks > 0)
              {
                thrdat.pworks_ += pworks;
                continue;
              }

              size_t mworks = 0;
              for (auto n : minors)
              {
                /// Try pop a worker to run.
                mworks += do_work(n, thrctx);
              }
              thrdat.mworks_ += mworks;
            }
          })
        );
    }

    /// Wait for join.
    for (auto& thr : thread_pool)
    {
      thr.join();
    }

    /// Clear thread pool.
    thread_pool.clear();
  }

  /// Stop ev_service.
  void stop()
  {
    /// Stop all thread.
    for (auto& thrdat : thread_data_list_)
    {
      thrdat.stop_ = true;
    }

    for (auto& thrdat : thread_data_list_)
    {
      thrdat.cnt_.synchronized_incr(thrdat.mtx_, thrdat.cv_);
    }
  }

  /// Get current thrctx.
  static thrctx_t* current()
  {
    return pri_current(nullptr);
  }

private:
  size_t do_work(size_t wkridx, thrctx_t& thrctx) noexcept
  {
    size_t works = 0;
    auto wkr = workshop_[wkridx].exchange(nullptr, std::memory_order_acq_rel);
    if (wkr != nullptr)
    {
      /// Set current worker.
      thrctx.set_worker(wkr);
      auto _ = gsl::finally(
        [this, wkridx, wkr, &thrctx]()
        {
          thrctx.set_worker(nullptr);
          workshop_[wkridx].store(wkr, std::memory_order_release);
          /// @note This must call, bcz before set wkr, there may be new event add into
          ///   this worker, but this worker's thread may exchange nullptr, so events may
          ///   be omited.
          notify_thread(wkridx);
        });
      works += wkr->work(thrctx);
    }
    return works;
  }

  static thrctx_t* pri_current(thrctx_t* thrctx = nullptr)
  {
    /// Local private thrctx.
    static thread_local thrctx_t* local_thrctx = nullptr;
    if (local_thrctx == nullptr && thrctx != nullptr)
    {
      local_thrctx = thrctx;
    }
    return local_thrctx;
  }

  void pri_async(size_t target, gsl::owner<event_base*> ev)
  {
    worker_list_[target].push_event(ev);
    notify_thread(target);
  }

  void pri_post(size_t target, post_handler_t&& hdr)
  {
    auto ev = make_event<detail::post_event>();
    ev->set_handler(std::move(hdr));
    pri_async(target, ev);
  }

  void pri_spawn(size_t target, coro_handler_t&& hdr, coctx::stack_size ssize)
  {
    using spawn_event_t = detail::spawn_event<ev_service>;
    auto ev = make_event<spawn_event_t>();
    ev->set_handler(std::move(hdr));
    ev->set_stack_size(ssize);
    pri_async(target, ev);
  }

  void pri_tstart(tstart_handler_t&& hdr)
  {
    tstart_handler_t h(hdr);
    for (auto& thrdat : thread_data_list_)
    {
      auto ev = make_event<detail::tstart_event>();
      ev->set_handler(h);
      thrdat.tstart_que_.push(ev);
    }
  }

  void pri_texit(texit_handler_t&& hdr)
  {
    texit_handler_t h(hdr);
    for (auto& thrdat : thread_data_list_)
    {
      auto ev = make_event<detail::texit_event>();
      ev->set_handler(h);
      thrdat.texit_que_.push(ev);
    }
  }

  void notify_thread(size_t wkridx)
  {
    auto thridx = wkridx % thread_data_list_.size();
    auto& thrdat = thread_data_list_[thridx];
    thrdat.cnt_.synchronized_incr(thrdat.mtx_, thrdat.cv_);
  }

  size_t select_strand_index() noexcept
  {
    auto sndidx = curr_sndidx_.load(std::memory_order_relaxed);
    auto result = sndidx;
    ++sndidx;
    if (sndidx >= worker_list_.size())
    {
      sndidx = 0;
    }
    curr_sndidx_.store(sndidx, std::memory_order_relaxed);
    return result;
  }

  coctx::context& get_hostctx(size_t index)
  {
    Expects(index < thread_data_list_.size());
    return thread_data_list_[index].host_ctx_;
  }

private:
  /// Local process unique id.
  uid_t uid_;

  /// Thread local pool.
  struct thread_local_pool : public cque::node_base
  {
    std::unique_ptr<cque::pool_base> pool_;
  };
  /// Thread local pool array.
  struct thread_local_pool_array
  {
    size_t size_;
    thread_local_pool* arr_[ASEV_MAX_EV_SERVICE];
  };
  using local_pool_queue_t = cque::mpsc_queue<thread_local_pool, eclipse_clock_t>;
  local_pool_queue_t local_pool_queue_;

  /// Each thread has a data.
  struct thread_data
  {
    thread_data(ev_service& evs, size_t index)
      : thrctx_(new thrctx_t(evs, index))
      , stop_(false)
    {
      pworks_ = 0;
      mworks_ = 0;
    }

    inline bool is_stop() const noexcept
    {
      return stop_.load(std::memory_order_relaxed);
    }

    cque::mpsc_count<eclipse_clock_t> cnt_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::unique_ptr<thrctx_t> thrctx_;
    cque::mpsc_queue<detail::tstart_event, eclipse_clock_t> tstart_que_;
    cque::mpsc_queue<detail::texit_event, eclipse_clock_t> texit_que_;

    int64_t pworks_;
    int64_t mworks_;

    CQUE_CACHE_ALIGNED_VAR(std::atomic_bool, stop_);
    CQUE_CACHE_ALIGNED_VAR(coctx::context, host_ctx_);
  };
  std::deque<thread_data> thread_data_list_;

  /// Workers.
  std::deque<detail::worker> worker_list_;
  std::deque<worker_ptr> workshop_;

  std::atomic_size_t curr_sndidx_;
  std::atomic_size_t stopped_workers_;
};
}
