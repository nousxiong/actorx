///
/// Test assertion.
///

#include <actorx/utest/all.hpp>
#include <actorx/assertion.hpp>

#include <iostream>
#include <cassert>


UTEST_CASE_SEQ(10, test_assertion)
{
  try
  {
    int i = 0;
    std::string str("std::cerr");
    ACTORX_ASSERTS(i == 0 && str == "hi")(i)(str).log().except();
#ifdef ACTORX_ENABLE_ASSERT
    Ensures(false);
#endif
  }
  catch (std::exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  std::cout << "------------------" << std::endl;

  try
  {
    int i = 0;
    std::string str("ensures");
    ACTORX_EXPECTS(i == 0 && str == "hi")(i)(str).except<std::runtime_error>();
    Ensures(false);
  }
  catch (std::runtime_error& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  std::cout << "------------------" << std::endl;

  try
  {
    ACTORX_ENSURES(false).msg("std::runtime_error").except<std::runtime_error>();
    Ensures(false);
  }
  catch (std::runtime_error& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  std::cout << "------------------" << std::endl;

  auto lg = spdlog::get("std_logger");
  if (!lg)
  {
    lg = spdlog::stdout_logger_mt("std_logger");
  }
  try
  {
    int i = 0;
    std::string str("std_logger");
    ACTORX_ASSERTS(i == 0 && str == "hi")(i)(str).log(lg).except();
#ifdef ACTORX_ENABLE_ASSERT
    Ensures(false);
#endif
  }
  catch (std::exception& ex)
  {
    lg->error(ex.what());
  }

  std::cout << "done." << std::endl;
}
