//
// Test main.
//

#include <actorx/utest/utest.hpp>


//UTEST_ENABLE(test_mailbox)
//UTEST_ENABLE(test_message)
//UTEST_ENABLE(test_pattern)
//UTEST_ENABLE(test_buffer_pool)
//UTEST_ENABLE(test_cow_buffer)
//UTEST_ENABLE(test_actor)
//UTEST_ENABLE(test_atom)
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
