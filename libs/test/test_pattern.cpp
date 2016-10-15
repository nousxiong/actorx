//
// Test actor.
//

#include <actorx/utest/all.hpp>

#include <actorx/all.hpp>

#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <memory>


UTEST_CASE(test_pattern)
{
  actx::pattern patt;

  auto aid1 = actx::make<actx::actor_id>("AXS", 0, 1, 2);
  auto aid2 = actx::make<actx::actor_id>("AXS", 0, 1, 3);
  patt.match("M1", "M2").guard(aid1, aid2);

  ACTX_ENSURES(patt.matches_.size() == 2);
  ACTX_ENSURES(patt.matches_[0] == actx::atom("M1"));
  ACTX_ENSURES(patt.matches_[1] == actx::atom("M2"));
  ACTX_ENSURES(patt.guards_.size() == 2);
  ACTX_ENSURES(patt.guards_[0] == aid1);
  ACTX_ENSURES(patt.guards_[1] == aid2);

  std::cout << "done.\n";
}
