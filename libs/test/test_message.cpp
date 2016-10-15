//
// Test message.
//

#include <actorx/utest/all.hpp>

#include <actorx/all.hpp>

#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <memory>


UTEST_CASE_SEQ(302, test_message)
{
  using namespace actx;

  detail::buffer_pool pool(2);
  auto msg1 = make<message>(&pool);

  auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
  auto aid1 = make<actor_id>("AXS", timestamp, 1, 1);
  ACTX_ENSURES(aid1 != nullaid);
  auto aid2 = make<actor_id>("AXS", timestamp, 1, 2);
  ACTX_ENSURES(aid2 != nullaid);
  ACTX_ENSURES(aid1 != aid2);

  auto str1 = std::make_shared<std::string>("string1");
  std::string str3 = "string3";

  msg1.put(aid1).put(str1).put(1LL).put(aid2).put(str3);

//  auto aid3 = make<actor_id>();
//  auto aid4 = make<actor_id>();

//  decltype(str1) str2;
//  decltype(str3) str4;
  auto aid3 = msg1.get<actor_id>();
  auto str2 = msg1.get_raw<std::string>();
  auto i = msg1.get<long long>();
  auto aid4 = msg1.get<actor_id>();
  auto str4 = msg1.get<std::string>();
//  msg1.read(aid3).readx(str2).read(i).read(aid4).read(str4);

  ACTX_ENSURES(aid3 == aid1);
  ACTX_ENSURES(str2 == str1);
  ACTX_ENSURES(i == 1LL);
  ACTX_ENSURES(aid4 == aid2);

  std::cout << "done.\n";
}
