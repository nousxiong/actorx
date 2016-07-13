//
// basic_corctx.hpp
//

#pragma once

#include "actorx/asev/config.hpp"
#include "actorx/asev/event_base.hpp"
#include "actorx/asev/affix_base.hpp"
#include "actorx/asev/detail/basic_thrctx.hpp"
#include "actorx/asev/detail/basic_strand.hpp"

#include <memory>
#include <type_traits>


namespace asev
{
namespace detail
{
template <class EvService>
class basic_corctx : public asev::event_base
{
  using strand_t = basic_strand<EvService>;
  using coro_handler_t = std::function<void (basic_corctx<EvService>&)>;

public:
  explicit basic_corctx(EvService& evs)
    : evs_(evs)
    , snd_(evs)
    , thrctx_(nullptr)
    , host_ctx_(nullptr)
    , stop_(false)
    , affix_(nullptr)
  {
  }

  ~basic_corctx()
  {
  }

public:
  inline EvService& get_ev_service()
  {
    return evs_;
  }

  inline strand_t& get_strand()
  {
    return snd_;
  }

  thrctx_t& get_thrctx()
  {
    ACTORX_ASSERTS(thrctx_ != nullptr);
    return *thrctx_;
  }

  void yield()
  {
    ACTORX_ASSERTS(host_ctx_ != nullptr);
    ACTORX_ASSERTS(!!ctx_);
    ACTORX_ASSERTS(thrctx_ != nullptr);
    thrctx_->set_corctx(nullptr);
    ctx_->jump(*host_ctx_);
  }

  void resume()
  {
    snd_.async(this);
  }

  template <typename T>
  void set_affix(T* affix)
  {
    static_assert(std::is_base_of<affix_base, T>::value, "T must inhert from asev::affix_base");
    affix_ = affix;
  }

  template <typename T>
  T* get_affix()
  {
    static_assert(std::is_base_of<affix_base, T>::value, "T must inhert from asev::affix_base");
    return gsl::narrow<T*>(affix_);
  }

public:
  // Internal use.
  void make_context(coro_handler_t hdr, coctx::stack_size ssize = coctx::make_stacksize())
  {
    hdr_ = std::move(hdr);
    if (!ctx_ || ctx_->get_stack_size() < ssize.size())
    {
      ctx_.reset(
        new coctx::context(
          [this](coctx::context&)
          {
            // Handler coroutine.
            try
            {
              hdr_(*this);
            }
            catch (...)
            {
            }

            // End self and yield.
            stop_ = true;
            snd_.async(this);
            yield();
          },
          ssize
          )
        );
    }
  }

  bool handle(thrctx_t& thrctx) override
  {
    thrctx_ = &thrctx;
    if (!stop_)
    {
      ACTORX_ASSERTS(!!hdr_);
      ACTORX_ASSERTS(!!ctx_);
      host_ctx_ = &evs_.get_hostctx(thrctx.get_index());
      thrctx.set_corctx(this);
      host_ctx_->jump(*ctx_);
    }
    else
    {
      // Release self to pool.
      cque::node_base::release();
    }
    return false;
  }

  void on_get(cque::freer_base* freer) noexcept override
  {
    asev::event_base::on_get(freer);
    stop_ = false;
  }

  void on_free() noexcept override
  {
    cque::node_base::on_free();
    host_ctx_ = nullptr;
    hdr_ = coro_handler_t();
    thrctx_ = nullptr;
    affix_ = nullptr;
    // No need reset, bcz ctx will reuse.
//    ctx_.reset();
  }

private:
  EvService& evs_;
  strand_t snd_;
  thrctx_t* thrctx_;

  coctx::context* host_ctx_;
  coro_handler_t hdr_;
  std::unique_ptr<coctx::context> ctx_;
  bool stop_;

  affix_base* affix_;
};

template <typename EvService>
struct corctx_make
{
  explicit corctx_make(EvService& evs) noexcept
    : evs_(evs)
  {
  }

  gsl::owner<cque::node_base*> operator()() const noexcept
  {
    return new (std::nothrow) basic_corctx<EvService>(evs_);
  }

  EvService& evs_;
};
} // namespace detail
} // namespace asev

