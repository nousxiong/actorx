//
// Test main.
//

#include <actorx/utest/utest.hpp>


//UTEST_ENABLE(test_strand)
//UTEST_ENABLE(test_pingpong)
//UTEST_ENABLE(test_spawn)
//UTEST_ENABLE(test_hook_sleep)
UTEST_ENABLE(test_hook_iocp)
//UTEST_ENABLE(test_assertion)
//UTEST_DISABLE(test_hook_iocp)
UTEST_DISABLE(test_hook_sleep)
UTEST_DISABLE(test_csegv)
UTEST_DISABLE(test_asev_csegv)
//UTEST_ENABLE(test_csegv)
//UTEST_ENABLE(test_asev_csegv)

UTEST_MAIN
