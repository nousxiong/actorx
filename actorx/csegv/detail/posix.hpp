//
// posix.hpp
//

#pragma once

#include "actorx/csegv/config.hpp"
#include "actorx/csegv/stack_info.hpp"

#include <list>
#include <vector>
#include <csignal>
#include <cstdlib>
#include <cstring>

#include <execinfo.h>
#include <unistd.h>
#include <dlfcn.h>


namespace csegv
{
namespace detail
{
static bool cut_file_line(char const* buff, int& length, int& line)
{
  length = 0;
  while (buff[length])
  {
    if (':' == buff[length])
    {
      line = std::atoi(buff + length + 1);
      return true;
    }
    length++;
  }
  return false;
}

#define CSEGV_ADDR2LINE "addr2line -f -e "

static std::list<stack_info> get_stack_list(
  void** traceback, size_t size, bool module, bool symbol, bool brief
  )
{
  std::list<stack_info> image_list;
  char cmd_head[256] = CSEGV_ADDR2LINE;
  char *prog = cmd_head + (sizeof(CSEGV_ADDR2LINE)-1);
  const int module_size = readlink("/proc/self/exe", prog, sizeof(cmd_head)-(prog - cmd_head) - 1);
  if (module_size > 0)
  {
//    const int prefix_size = (sizeof(CSEGV_ADDR2LINE)-1) + module_size;
    std::string cmd = cmd_head;
    for (size_t i = 0; i < size; i++)
    {
      char tmp[24];
      std::snprintf(tmp, sizeof(tmp), " %p", traceback[i]);
      cmd.append(tmp);
    }

    FILE* fp = popen(cmd.c_str(), "r");
    if (fp)
    {
      char buff[1024] = "\0";
      while (true)
      {
        stack_info stack_result(brief);
        if (std::fgets(buff, sizeof(buff), fp) != nullptr)
        {
          if (symbol)
          {
            stack_result.symbol_ = buff;
            if (!stack_result.symbol_.empty())
            {
              stack_result.symbol_.resize(stack_result.symbol_.size()-1);
            }
          }

          if (module)
          {
            stack_result.module_ = std::string(cmd_head + (sizeof(CSEGV_ADDR2LINE)-1), module_size);
          }

          if (std::fgets(buff, sizeof(buff), fp) != nullptr)
          {
            int length, line;
            if (cut_file_line(buff, length, line))
            {
              stack_result.line_ = line;
              stack_result.file_ = std::string(buff, length);
            }
            else
            {
              stack_result.line_ = -1;
              stack_result.file_ = buff;
            }
            image_list.push_back(std::move(stack_result));
          }
          else
          {
            break;
          }
        }
        else
        {
          break;
        }
      }
      pclose(fp);
    }
  }
  return image_list;
}

static std::list<stack_info> get_stack_list(
  void* reg_bp, void*, void* reg_ip,
  size_t stack_depth, size_t offset, bool module, bool symbol, bool brief
  )
{
  std::vector<void*> traceback;

  Dl_info dlinfo;
  void* ip = reg_ip;
  void** bp = (void**)reg_bp;
  while (&ip != bp && traceback.size() < stack_depth)
  {
    if (dladdr(ip, &dlinfo) == 0)
    {
      break;
    }

    traceback.push_back(ip);
    if (!bp)
    {
      break;
    }

    ip = bp[1];
    bp = (void**)(*bp);
  }

  if (traceback.size() > offset)
  {
    return get_stack_list(&traceback[offset], traceback.size() - offset, module, symbol, brief);
  }
  return std::list<stack_info>();
}

// Using backtrace to get stack list.
static std::list<stack_info> get_stack_list(size_t stack_depth)
{
  ACTX_ASSERTS(stack_depth <= 32);

  std::list<stack_info> stack_info_list;
  void* trace_buf[32];
  auto trace_size = backtrace(trace_buf, stack_depth);
  auto trace_infos = backtrace_symbols(trace_buf, trace_size);

  auto _ = gsl::finally(
    [trace_infos]()
    {
      if (trace_infos != nullptr)
      {
        std::free(trace_infos);
      }
    });

  if (trace_infos == nullptr || (*trace_infos) == nullptr)
  {
    return stack_info_list;
  }

  for(size_t i=0; i<trace_size; ++i)
  {
    stack_info stk;
    stk.module_ = trace_infos[i];
    stack_info_list.push_back(std::move(stk));
  }
  return stack_info_list;
}

struct context
{
  static context& get()
  {
    static thread_local context ctx;
    return ctx;
  }

  static size_t constexpr const size_ = 8 * 1024;
  unsigned char stack_[size_];

  // Local templ vars.
  size_t stack_depth_;
  bool brief_;
  handler_t h_;
};

static void signal_filter(context& ctx)
{
  stack_t sigstk;
  sigstk.ss_size = ctx.size_;
  sigstk.ss_sp = ctx.stack_;
  sigstk.ss_flags = 0;
  sigaltstack(&sigstk, nullptr);

  struct sigaction sigact;
  std::memset(&sigact, 0, sizeof(sigact));
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigact.sa_sigaction =
    [](int signum, siginfo_t* info, void* ptr)
    {
      auto& ctx = context::get();
      auto stack_depth = ctx.stack_depth_;

#if defined(REG_RIP) || defined(REG_EIP)
      auto ucontext = (ucontext_t*)ptr;
# ifdef REG_RIP
      auto reg_bp = REG_RBP;
      auto reg_ip = REG_RIP;
# elif REG_EIP
      auto reg_bp = REG_EBP;
      auto reg_ip = REG_EIP;
# endif
      auto stack_list =
        detail::get_stack_list(
          (void*)ucontext->uc_mcontext.gregs[reg_bp], nullptr,
          (void*)ucontext->uc_mcontext.gregs[reg_ip],
          stack_depth, 0, true, true, ctx.brief_
        );
#else
      auto stack_list = detail::get_stack_list(stack_depth);
#endif

      auto h = std::move(ctx.h_);
      if (h)
      {
        h(stack_list);
      }
      else
      {
        for (auto& ele : stack_list)
        {
          std::cerr << ele << std::endl;
        }
        std::cerr << std::endl << std::flush;
      }
      std::exit(102);
    };
  sigaction(SIGSEGV, &sigact, nullptr);
}
} // namespace detail
} // namespace csegv
