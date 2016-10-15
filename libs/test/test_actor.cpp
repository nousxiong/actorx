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


UTEST_CASE(test_actor)
{
  actx::ax_service axs("AXS");
  auto logger = axs.get_logger();

  // Start axs.
  axs.startup();

  // Spawn an thread actor.
  actx::actor base_ax = axs.spawn();

  // Spawn an coroutine based actor and monitor it.
  auto taid = axs.spawn(
    base_ax,
    [logger](actx::actor self)
    {
      auto msg = self.recv("INIT");
      auto base_aid = msg.get_sender();
      auto what = msg.get<std::string>();
      logger->info("Recv from base_ax: {}", what);

      std::string reply(what.rbegin(), what.rend());

      // Send reply.
      self.send(base_aid, "REPLY", reply);
    },
    actx::fiber_meta()
      .link(actx::link_type::monitored)
      .stack_size(1024 * 256)
      .sync_sire(false)
    );

  ACTX_ENSURES(taid != actx::nullaid);
  base_ax.send(taid, "INIT", "Hello World!");

  auto msg =
    base_ax.recv(
      actx::pattern()
      .match("REPLY")
      .guard(taid)
      .after(std::chrono::seconds(1))
      );

  if (msg == actx::nullmsg)
  {
    // Timed out.
    ACTX_ENSURES(false);
  }
  ACTX_ENSURES(msg.get_sender() == taid);
  auto reply = msg.get<std::string>();
  ACTX_ENSURES(reply == "!dlroW olleH");

  auto emsg = base_ax.recv(actx::msg_type::exit);
  ACTX_ENSURES(emsg.get_sender() == taid);
  ACTX_ENSURES(
    emsg.get<actx::actor_exit>().type ==
    static_cast<int8_t>(actx::exit_type::normal)
    );

  // Shut axs and wait it quit.
  axs.shutdown();
  logger->info("done.");
}
