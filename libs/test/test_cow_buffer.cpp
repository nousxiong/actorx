//
// Test cow_buffer.
//

#include <actorx/utest/all.hpp>

#include <actorx/all.hpp>

#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <memory>


UTEST_CASE_SEQ(301, test_cow_buffer)
{
  actx::detail::buffer_pool pool(1);
  actx::detail::cow_buffer buf1(&pool);
  ACTX_ENSURES(buf1.is_small());

  size_t const small_size = ACTX_SMALL_BUFFER_SIZE;

  std::vector<actx::byte_t> v1(small_size/2, 1);
  auto s1 = gsl::as_span(v1);
  ACTX_ENSURES(buf1.reserve(s1.size()));
  ACTX_ENSURES(buf1.write(s1.size()) == small_size/2);
  ACTX_ENSURES(buf1.is_small());

  std::vector<actx::byte_t> v2(small_size + 1, 2);
  auto s2 = gsl::as_span(v2);
  ACTX_ENSURES(buf1.reserve(s2.size()));
  ACTX_ENSURES(buf1.write(s2.size()) == small_size + 1);
  ACTX_ENSURES(!buf1.is_small());

  actx::detail::cow_buffer buf2(buf1);
  ACTX_ENSURES(!buf2.is_small());

  std::vector<actx::byte_t> v3(small_size/2);
  auto s3 = gsl::as_span(v3);
  ACTX_ENSURES(buf1.read(s3.size()) == small_size/2);
//  ACTX_ENSURES(v3 == v1);

  std::vector<actx::byte_t> v4(small_size + 1);
  auto s4 = gsl::as_span(v4);
  ACTX_ENSURES(buf1.read(s4.size()) == small_size + 1);
//  ACTX_ENSURES(v4 == v2);

  // Copy-on-write.
  std::vector<actx::byte_t> v5(small_size + 1, 3);
  auto s5 = gsl::as_span(v5);
  ACTX_ENSURES(buf1.reserve(s5.size()));
  ACTX_ENSURES(buf1.write(s5.size()) == small_size + 1);

  std::vector<actx::byte_t> v6(small_size + 1);
  auto s6 = gsl::as_span(v6);
  ACTX_ENSURES(buf1.read(s6.size()) == small_size + 1);
//  ACTX_ENSURES(v6 == v5);

  std::cout << "done.\n";
}
