//
// Test hook win32 api Sleep.
//

#include <actorx/utest/all.hpp>


#ifdef _MSC_VER

#include <xhook.hpp>
#include <actorx/asev/all.hpp>

#include <iostream>


typedef void(WINAPI *SleepF)(DWORD dwMilliseconds);
SleepF sleep_fn = &::Sleep;

void WINAPI MySleep(DWORD ms)
{
  std::cout << "MySleep(" << ms << ")" << std::endl;
}

UTEST_CASE(test_hook_sleep)
{
  asev::ev_service evs(1);
  evs.tstart(
    [](asev::thrctx_t& thrctx)
    {
      XHookRestoreAfterWith();
      XHookTransactionBegin();
      XHookUpdateThread(GetCurrentThread());
      XHookAttach(&(PVOID&)sleep_fn, MySleep);
      XHookTransactionCommit();
      ::Sleep(1);
    });
  evs.post([](asev::thrctx_t& thrctx) { thrctx.get_ev_service().stop(); });
  evs.run();
  std::cout << "done." << std::endl;
}

#endif // _MSC_VER
