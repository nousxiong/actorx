///
/// assertion.hpp
///

#pragma once

#include "actorx/config.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <iostream>
#include <array>
#include <type_traits>
#include <exception>
#include <string>
#include <cstdio>
#include <cassert>


namespace actorx
{
template <typename T>
struct format
{
};

enum assert_type
{
  asserts,
  expects,
  ensures
};

class assert_except
  : public virtual std::exception
{
#ifdef UTEST_SYSTEM_CLOCK
  using eclipse_clock_t = std::chrono::system_clock;
#else
  using eclipse_clock_t = std::chrono::steady_clock;
#endif // UTEST_SYSTEM_CLOCK
  using time_point_t = eclipse_clock_t::time_point;

public:
  assert_except(time_point_t tp, gsl::czstring msg, gsl::czstring err, assert_type aty)
  {
    if (err != nullptr)
    {
      what_.append(err);
    }
    gsl::czstring desc = nullptr;
    switch (aty)
    {
    case assert_type::expects: desc = "Expects exception, timestamp <"; break;
    case assert_type::ensures: desc = "Ensures exception, timestamp <"; break;
    default: desc = "Asserts exception, timestamp <"; break;
    }
    make_description(what_, desc, tp, msg);
  }


  virtual ~assert_except() throw()
  {
  }

  gsl::czstring what() const throw()
  {
    return what_.c_str();
  }

  static void make_description(fmt::MemoryWriter& buf, gsl::czstring prefix, time_point_t tp, gsl::czstring msg)
  {
    buf.write(prefix);
    buf << tp.time_since_epoch().count();
    buf.write(">");
    if (msg != nullptr)
    {
      buf.write(", \"");
      buf.write(msg);
      buf.write("\"");
    }
  }

  static void make_description(std::string& buf, gsl::czstring prefix, time_point_t tp, gsl::czstring msg)
  {
    buf.append(prefix);
    char numstr[32] = "\0";
    std::snprintf(numstr, 32, "%lld", (long long)tp.time_since_epoch().count());
    buf.append(numstr);
    buf.append(">");
    if (msg != nullptr)
    {
      buf.append(", \"");
      buf.append(msg);
      buf.append("\"");
    }
  }

private:
  std::string what_;
};

namespace detail
{
class assertion
{
#ifdef UTEST_SYSTEM_CLOCK
  using eclipse_clock_t = std::chrono::system_clock;
#else
  using eclipse_clock_t = std::chrono::steady_clock;
#endif // UTEST_SYSTEM_CLOCK
  using time_point_t = eclipse_clock_t::time_point;
  using errmsg_t = fmt::MemoryWriter;
  using logger_ptr = std::shared_ptr<spdlog::logger>;

  enum log_type
  {
    log_default,
    log_std,
    log_actorx
  };

  enum handle_type
  {
    handle_default,
    handle_excepted,
    handle_aborted
  };

  struct enum_t {};
  struct ptr_t {};
  struct other_t {};

  assertion(assertion const&) = delete;
  assertion& operator=(assertion const&) = delete;

public:
  assertion(
    gsl::czstring expr, gsl::czstring file, int line,
    spdlog::level::level_enum lvl, assert_type aty = assert_type::asserts
    )
    : ACTORX_ASSERT_A(*this)
    , ACTORX_ASSERT_B(*this)
    , lvl_(lvl)
    , ex_((time_point_t::min)())
    , msg_(nullptr)
    , lg_type_(log_default)
    , hdl_type_(handle_default)
    , aty_(aty)
    , is_throw_(expr == "false" || expr == "0" || expr == "NULL")
    , moved_(false)
  {
    if (is_throw_)
    {
      str_.write("Throw exception in ");
    }
    else
    {
      gsl::czstring desc = nullptr;
      switch (aty)
      {
      case assert_type::expects: desc = "Expects failed in "; break;
      case assert_type::ensures: desc = "Ensures failed in "; break;
      default: desc = "Asserts failed in "; break;
      }
      str_.write(desc);
    }
    str_.write(file);
    str_.write(": ");
    str_ << line;
    if (is_throw_)
    {
      str_.write("\n");
    }
    else
    {
      str_.write("\nExpression: '");
      str_.write(expr);
      str_.write("'\n");
    }
  }

  assertion(assertion&& other)
    : ACTORX_ASSERT_A(*this)
    , ACTORX_ASSERT_B(*this)
    , str_(std::move(other.str_))
    , lg_(std::move(other.lg_))
    , lvl_(other.lvl_)
    , ex_(std::move(other.ex_))
    , msg_(std::move(other.msg_))
    , lg_type_(other.lg_type_)
    , hdl_type_(other.hdl_type_)
    , aty_(other.aty_)
    , is_throw_(other.is_throw_)
    , moved_(false)
  {
    other.moved_ = true;
  }

  ~assertion() noexcept(false) // Under gcc -std=c++11, default is noexcept(true), so must set false.
  {
    if (moved_)
    {
      return;
    }

    if (msg_ != nullptr)
    {
      str_.write(msg_);
      str_.write("\n");
    }

    if (ex_ != (time_point_t::min)())
    {
      actorx::assert_except::make_description(str_, "Exception timestamp <", ex_, msg_);
    }

    bool throw_except = false;
    switch (hdl_type_)
    {
    case handle_default:
      {
        switch (lvl_)
        {
        case spdlog::level::trace:
        case spdlog::level::debug:
          pri_abort();
          break;
        case spdlog::level::info:
        case spdlog::level::notice:
        case spdlog::level::warn:
          break;
        case spdlog::level::err:
          {
            throw_except = true;
            ex_ = eclipse_clock_t::now();
          }break;
        case spdlog::level::critical:
        case spdlog::level::alert:
        case spdlog::level::emerg:
        case spdlog::level::off:
          pri_abort();
          break;
        default:
          assert(false);
          break;
        }
      }break;
    case handle_aborted:
      pri_abort();
      break;
    default:
      break;
    }

    auto log_str = str_.c_str();
    switch (lg_type_)
    {
    case log_std: std::cerr << log_str << std::endl; break;
    case log_actorx: lg_->force_log(lvl_, log_str); break;
    default: break;
    }

    if (throw_except)
    {
      if (lg_type_ != log_default)
      {
        log_str = nullptr;
      }
      throw actorx::assert_except(ex_, msg_, log_str, aty_);
    }
  }

  assertion& ACTORX_ASSERT_A;
  assertion& ACTORX_ASSERT_B;

public:
  assertion& msg(gsl::czstring msg)
  {
    msg_ = msg;
    return *this;
  }

  assertion& log(gsl::czstring msg = nullptr)
  {
    lg_type_ = log_std;
    msg_ = msg;
    return *this;
  }

  assertion& log(logger_ptr lg, gsl::czstring msg = nullptr)
  {
    return log(lg, lvl_, msg);
  }

  assertion& log(logger_ptr lg, spdlog::level::level_enum lvl, gsl::czstring msg = nullptr)
  {
    assert(!!lg);
    lg_ = lg;
    lg_type_ = log_actorx;
    lvl_ = lvl;
    msg_ = msg;
    return *this;
  }

  void except()
  {
    auto log_str = pri_except();
    throw actorx::assert_except(ex_, msg_, log_str, aty_);
  }

  void except(std::error_code const& ec)
  {
    auto log_str = pri_except();
    actorx::assert_except ex(ex_, msg_, log_str, aty_);
    throw std::system_error(ec, ex.what());
  }

  template <typename Except>
  void except()
  {
    auto log_str = pri_except();
    actorx::assert_except ex(ex_, msg_, log_str, aty_);
    throw Except(ex.what());
  }

  void abort()
  {
    hdl_type_ = handle_aborted;
  }

private:
  gsl::czstring pri_except()
  {
    ex_ = eclipse_clock_t::now();
    hdl_type_ = handle_excepted;
    gsl::czstring log_str = nullptr;
    if (lg_type_ == log_default)
    {
      log_str = str_.c_str();
    }
    return log_str;
  }

public:
  /// internal use
  assertion& set_var(char v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(gsl::czstring v, gsl::czstring name)
  {
    str_.write("  ");
    str_.write(name);
    str_.write(" = ");
    str_.write(v);
    str_.write("\n");
    return *this;
  }

  assertion& set_var(std::string const& v, gsl::czstring name)
  {
    str_.write("  ");
    str_.write(name);
    str_.write(" = ");
    str_.write(v.c_str());
    str_.write("\n");
    return *this;
  }

  assertion& set_var(gsl::cstring_span v, gsl::czstring name)
  {
    str_.write("  ");
    str_.write(name);
    str_.write(" = ");
    str_.write(v.data(), v.size());
    str_.write("\n");
    return *this;
  }

  assertion& set_var(gsl::string_span v, gsl::czstring name)
  {
    str_.write("  ");
    str_.write(name);
    str_.write(" = ");
    str_.write(v.data(), v.size());
    str_.write("\n");
    return *this;
  }

  assertion& set_var(bool v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(signed char v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(unsigned char v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(short v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(unsigned short v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(int v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(unsigned int v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(long v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(unsigned long v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(long long v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(unsigned long long v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(float v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(double v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  assertion& set_var(long double v, gsl::czstring name)
  {
    return pri_set_var(v, name);
  }

  template<typename Rep, typename Ratio>
  assertion& set_var(std::chrono::duration<Rep, Ratio> v, gsl::czstring name)
  {
    return pri_set_var(v.count(), name);
  }

  template <typename T>
  assertion& set_var(T const& v, gsl::czstring name)
  {
    typedef typename std::decay<T>::type param_t;

    typedef typename std::conditional<
      std::is_enum<param_t>::value, enum_t, other_t
      >::type select_enum_t;

    typedef typename std::conditional<
      std::is_pointer<param_t>::value, ptr_t, select_enum_t
    >::type select_ptr_t;

    pri_set_var(select_ptr_t(), v, name);
    return *this;
  }

private:
  template <typename T>
  assertion& pri_set_var(T const v, gsl::czstring name)
  {
    str_.write("  ");
    str_.write(name);
    str_.write(" = ");
    str_ << v;
    str_.write("\n");
    return *this;
  }

  template <typename T>
  assertion& pri_set_var(enum_t, T const v, gsl::czstring name)
  {
    str_.write("  ");
    str_.write(name);
    str_.write(" = ");
    str_.write("{}", v);
    str_.write("\n");
    return *this;
  }

  template <typename T>
  assertion& pri_set_var(other_t, T const& v, gsl::czstring name)
  {
    str_.write("  ");
    str_.write(name);
    str_.write(" = ");
    str_.write(format<T>::cast(v).c_str());
    str_.write("\n");
    return *this;
  }

  template <typename T>
  assertion& pri_set_var(ptr_t, T v, gsl::czstring name)
  {
    str_.write("  ");
    str_.write(name);
    str_.write(" = ");
    str_.write("{}", v);
    str_.write("\n");
    return *this;
  }

  void pri_abort()
  {
    auto log_str = str_.c_str();
    switch (lg_type_)
    {
    case log_actorx:
      if (lg_)
      {
        lg_->force_log(lvl_, log_str);
        break;
      }
    case log_std:
    default:
      std::cerr << log_str << std::endl;
      break;
    }
    std::abort();
  }

private:
  errmsg_t str_;
  logger_ptr lg_;
  spdlog::level::level_enum lvl_;
  time_point_t ex_;
  gsl::czstring msg_;
  log_type lg_type_;
  handle_type hdl_type_;
  assert_type aty_;
  bool is_throw_;
  bool moved_;
};

inline assertion make_assert(
  gsl::czstring expr, gsl::czstring file, int line,
  spdlog::level::level_enum lvl, assert_type aty = assert_type::asserts
  )
{
  return std::move(assertion(expr, file, line, lvl, aty));
}
} /// namespace detail
} /// namespace actorx

#define ACTORX_ASSERT_A(x) ACTORX_ASSERT_OP(x, B)
#define ACTORX_ASSERT_B(x) ACTORX_ASSERT_OP(x, A)

#define ACTORX_ASSERT_OP(x, next) \
  ACTORX_ASSERT_A.set_var((x), #x).ACTORX_ASSERT_##next

#ifdef ACTORX_ENABLE_ASSERT
# define ACTORX_ASSERTS(expr) \
  if ( (expr) ) ; \
  else actorx::detail::make_assert(#expr, __FILE__, __LINE__, spdlog::level::debug).ACTORX_ASSERT_A

#else
# define ACTORX_ASSERTS(expr) \
  if ( true ) ; \
  else actorx::detail::make_assert(#expr, __FILE__, __LINE__, spdlog::level::debug).ACTORX_ASSERT_A

#endif

# define ACTORX_EXPECTS(expr) \
  if ( (expr) ) ; \
  else actorx::detail::make_assert(#expr, __FILE__, __LINE__, spdlog::level::err, actorx::assert_type::expects).ACTORX_ASSERT_A

# define ACTORX_ENSURES(expr) \
  if ( (expr) ) ; \
  else actorx::detail::make_assert(#expr, __FILE__, __LINE__, spdlog::level::err, actorx::assert_type::ensures).ACTORX_ASSERT_A
