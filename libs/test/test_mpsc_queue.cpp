///
/// Test mpsc_queue.
///

#include <actorx/utest/all.hpp>
#include <actorx/cque/all.hpp>

#include <chrono>
#include <thread>
#include <vector>
#include <deque>
#include <iostream>
#include <type_traits>


UTEST_CASE_SEQ(220, test_mpsc_queue)
{
  struct data : public cque::node_base
  {
    size_t i_;
    size_t t_;
    std::string str_;
  };

  size_t count = 100000;
  size_t concurr = std::thread::hardware_concurrency();
  auto total = count * concurr;

  std::vector<std::thread> thrs;
  std::vector<size_t> counter(concurr, 0);
  thrs.reserve(concurr);

  std::deque<cque::mpsc_pool<data>> pools;
  for (size_t i=0; i<concurr; ++i)
  {
    pools.emplace_back();
    auto& pool = pools.back();
    // Pre alloc
    for (size_t n=0; n<count; ++n)
    {
      pool.free(new data);
    }
  }

  cque::mpsc_queue<data> que;

  auto bt = std::chrono::high_resolution_clock::now();
  for (size_t t=0; t<concurr; ++t)
  {
    auto& pool = pools[t];
    thrs.push_back(
      std::thread{
        [count, t, &que, &pool]()
        {
          for (size_t i=0; i<count; ++i)
          {
            auto e = cque::get<data>(pool);
            e->i_ = i;
            e->t_ = t;
            e->str_ = "test string";
            que.push(e);
          }
        }}
      );
  }

  for (size_t i=0; i<total; )
  {
    auto e = que.pop_unique<cque::pool_delete<data>>();
    if (e)
    {
      ACTORX_ENSURES(e->t_ >= 0 && e->t_ < concurr)(e->t_);
      ACTORX_ENSURES(counter[e->t_] == e->i_)(counter[e->t_])(e->i_);
      ACTORX_ENSURES(e->str_ == "test string")(e->str_);
      ++counter[e->t_];
      ++i;
    }
  }

  for (auto& thr : thrs)
  {
    thr.join();
  }

  auto et = std::chrono::high_resolution_clock::now();
  auto time_span =
    std::chrono::duration_cast<std::chrono::duration<double>>(et - bt);
  std::cout << "done, eclipse: " << time_span.count() << " s" << std::endl;
}

UTEST_CASE_SEQ(221, test_mpsc_queue_variant)
{
  struct data : public cque::node_base
  {
    size_t i_;
    size_t t_;
    std::string str_;
  };

  size_t count = 100000;
  size_t concurr = std::thread::hardware_concurrency();
  auto total = count * concurr;

  std::vector<std::thread> thrs;
  std::vector<size_t> counter(concurr, 0);
  thrs.reserve(concurr);

  std::deque<cque::mpsc_pool<data>> pools;
  for (size_t i=0; i<concurr; ++i)
  {
    pools.emplace_back();
    auto& pool = pools.back();
    // Pre alloc
    for (size_t n=0; n<count; ++n)
    {
      pool.free(new data);
    }
  }

  cque::mpsc_queue<data> que;

  auto bt = std::chrono::high_resolution_clock::now();
  for (size_t t=0; t<concurr; ++t)
  {
    auto& pool = pools[t];
    thrs.push_back(
      std::thread{
        [count, t, &que, &pool]()
        {
          for (size_t i=0; i<count; ++i)
          {
            auto mod = i % 3;
            if (mod == 0)
            {
              auto e = cque::get<data>(pool);
              e->i_ = i;
              e->t_ = t;
              e->str_ = "test string";
              que.push(e);
            }
            else if (mod == 1)
            {
              auto e = cque::get_shared<data>(pool);
              e->i_ = i;
              e->t_ = t;
              e->str_ = "test string";
              que.push(std::move(e));
            }
            else
            {
              auto e = cque::get_unique<data>(pool);
              e->i_ = i;
              e->t_ = t;
              e->str_ = "test string";
              que.push(std::move(e));
            }
          }
        }}
      );
  }

  for (size_t i=0; i<total; )
  {
    auto e = que.pop_variant<cque::pool_delete<data>>();
    if (e)
    {
      ACTORX_ENSURES(e->t_ >= 0 && e->t_ < concurr)(e->t_);
      ACTORX_ENSURES(counter[e->t_] == e->i_)(counter[e->t_])(e->i_);
      ACTORX_ENSURES(e->str_ == "test string")(e->str_);
      ++counter[e->t_];
      ++i;
    }
  }

  for (auto& thr : thrs)
  {
    thr.join();
  }

  auto et = std::chrono::high_resolution_clock::now();
  auto time_span =
    std::chrono::duration_cast<std::chrono::duration<double>>(et - bt);
  std::cout << "done, eclipse: " << time_span.count() << " s" << std::endl;
}
