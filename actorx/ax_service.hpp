//
// ax_service.hpp
//

#pragma once

#include "actorx/config.hpp"
#include "actorx/atom.hpp"
#include "actorx/actor/actor.hpp"
#include "actorx/actor/fiber_meta.hpp"
#include "actorx/detail/fiber_actor.hpp"
#include "actorx/detail/buffer_pool.hpp"
#include "actorx/detail/actor_pool.hpp"

#include "actorx/asev/all.hpp"

#include <spdlog/spdlog.h>

#include <thread>
#include <functional>
#include <memory>


namespace actx
{
//! Service of actorx.
class ax_service
{
  ax_service(ax_service const&) = delete;
  ax_service(ax_service&&) = delete;
  ax_service& operator=(ax_service const&) = delete;
  ax_service& operator=(ax_service&&) = delete;

  using logger_ptr = std::shared_ptr<spdlog::logger>;
  using actor_handler_t = std::function<void (actor)>;

#ifdef ASEV_SYSTEM_CLOCK
  using eclipse_clock_t = std::chrono::system_clock;
#else
  using eclipse_clock_t = std::chrono::steady_clock;
#endif // ASEV_SYSTEM_CLOCK

public:
  explicit ax_service(gsl::czstring axid)
    : axid_(dynatom(axid))
    , timestamp_(eclipse_clock_t::now().time_since_epoch().count())
  {
  }

  template <size_t TypeSize>
  explicit ax_service(char const (&axid)[TypeSize])
    : axid_(atom(axid))
    , timestamp_(eclipse_clock_t::now().time_since_epoch().count())
  {
  }

  ~ax_service()
  {
  }

public:
  //! Startup with default args.
  void startup()
  {
    startup(std::thread::hardware_concurrency(), logger_ptr(), 0, true);
  }

  //! Startup with given thread nums.
  void startup(size_t thread_num)
  {
    startup(thread_num, logger_ptr(), 0, true);
  }

  //! Startup with given logger.
  void startup(logger_ptr logger)
  {
    startup(std::thread::hardware_concurrency(), logger, 0, true);
  }

  //! Startup with given work_steal opt.
  void startup(asev::work_steal ws)
  {
    startup(std::thread::hardware_concurrency(), logger_ptr(), 0, ws);
  }

  //! Startup with given args.
  void startup(size_t thread_num, logger_ptr logger, size_t worker_num, bool work_steal)
  {
    ACTX_ASSERTS(!evs_);
    evs_.reset(new asev::ev_service(thread_num, logger, worker_num, work_steal));
    buffer_pool_.reset(new detail::buffer_pool(evs_->get_uid()));
    fiber_actor_pool_.reset(
      new detail::actor_pool<detail::fiber_actor>(
        evs_->get_uid(), buffer_pool_.get(), axid_, timestamp_
        )
      );

    evs_thr_ = std::thread(
      [this]()
      {
        evs_->run();
      });
  }

  //! Shutdown and wait for end.
  void shutdown() noexcept
  {
    ACTX_ASSERTS(!!evs_);
    evs_->stop();
    evs_thr_.join();
  }

  //! Get axid.
  atom_t get_axid() const noexcept
  {
    return axid_;
  }

  int64_t get_timestamp() const noexcept
  {
    return timestamp_;
  }

  //! Get logger.
  logger_ptr get_logger() const noexcept
  {
    if (evs_)
    {
      return evs_->get_logger();
    }
    else
    {
      return logger_ptr();
    }
  }

public:
  //! Spawn an actor and schedule it by user.
  actor spawn(actor sire = nullactor, link_type lty = link_type::no_link)
  {
    return nullactor;
  }

  template <typename F>
  actor_id spawn(F&& f, fiber_meta meta)
  {
    return spawn(nullactor, std::forward<F>(f), meta);
  }

  template <typename F>
  actor_id spawn(actor sire, F&& f, fiber_meta meta)
  {
    // Create actor handler.
    actor_handler_t hdr(f);

    // Generate actor_id.
    auto aid = generate_actor_id();

    // Spawn an coctx.
    asev::strand_t snd(*evs_);
    auto sire_aid = sire == nullactor ? nullaid : sire.get_actor_id();
    if (sire_aid != nullaid && meta.ssire_)
    {
      snd = sire.get_strand();
    }

    snd.spawn(
      [this, sire_aid, hdr, aid](asev::corctx_t& ctx)
      {
        // Alloc fiber_actor.
        auto a = fiber_actor_pool_->get();
        // Run actor.
        a->init(aid, ctx.get_strand());
        a->run(ctx, sire_aid, std::move(hdr));
      },
      coctx::make_stacksize(meta.ssize_)
      );
    return aid;
  }

private:
  actor_id generate_actor_id()
  {
    return nullaid;
  }

private:
  atom_t const axid_;
  int64_t const timestamp_;
  std::unique_ptr<asev::ev_service> evs_;
  std::unique_ptr<detail::buffer_pool> buffer_pool_;
  std::unique_ptr<detail::actor_pool<detail::fiber_actor>> fiber_actor_pool_;

  // Thread of runing ev_service.
  std::thread evs_thr_;
};
} // namespace actx
