///
/// posix.hpp
///

#pragma once

#include "actorx/csegv/config.hpp"
#include "actorx/csegv/detail/stack.hpp"

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
  void** traceback, size_t size, bool module, bool symbol
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
        stack_info stack_result;
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
  size_t map_depth, size_t offset, bool module, bool symbol
  )
{
  auto stack_depth = max_stack_depth();
  Expects(map_depth && map_depth <= stack_depth);
  std::vector<void*> traceback;

  Dl_info dlinfo;
  void* ip = reg_ip;
  void** bp = (void**)reg_bp;
  while (bp && ip && traceback.size() < stack_depth)
  {
    if (!dladdr(ip, &dlinfo))
    {
      break;
    }
    traceback.push_back(ip);
    ip = bp[1];
    bp = (void**)bp[0];
  }

  if (traceback.size() > offset)
  {
    return get_stack_list(&traceback[offset], traceback.size() - offset, module, symbol);
  }
  return std::list<stack_info>();
}

struct context
{
  bool inited_;
  static size_t constexpr const size_ = 8 * 1024;
  unsigned char stack_[size_];
};

static void signal_filter(context& ctx)
{
  if (ctx.inited_)
  {
    return;
  }

  stack_t sigaltStack;
  sigaltStack.ss_size = ctx.size_;
  sigaltStack.ss_sp = ctx.stack_;
  sigaltStack.ss_flags = 0;
  sigaltstack(&sigaltStack, nullptr);

  struct sigaction sigact;
  std::memset(&sigact, 0, sizeof(sigact));
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigact.sa_sigaction =
    [](int signum, siginfo_t* info, void* ptr)
    {
      auto stack_depth = max_stack_depth();
      auto ucontext = (ucontext_t*)ptr;
# ifdef REG_RIP
      auto reg_bp = REG_RBP;
      auto reg_ip = REG_RIP;
# elif REG_EIP
      auto reg_bp = REG_EBP;
      auto reg_ip = REG_EIP;
# endif
      auto stk =
        detail::get_stack_list(
          (void*)ucontext->uc_mcontext.gregs[reg_bp], nullptr,
          (void*)ucontext->uc_mcontext.gregs[reg_ip], stack_depth, 0, true, true
        );

      for (auto& ele : stk)
      {
        std::cout << ele << std::endl;
      }
      std::exit(102);
    };
  sigaction(SIGSEGV, &sigact, nullptr);

  ctx.inited_ = true;
}
} /// namespace detail
} /// namespace csegv
