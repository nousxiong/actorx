///
/// windows.hpp
///

#pragma once

#include "actorx/csegv/config.hpp"
#include "actorx/csegv/detail/stack.hpp"

#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <csignal>
#include <iostream>
#include <cassert>

#include <windows.h>
#include <DbgHelp.h>
#include <WinDNS.h>


namespace csegv
{
namespace detail
{
static size_t stack_walk(QWORD* pTrace, size_t max_depth, CONTEXT* pContext)
{
  STACKFRAME64 stackFrame64;
  HANDLE hProcess = GetCurrentProcess();
  HANDLE hThread = GetCurrentThread();

  size_t depth = 0;

  ZeroMemory(&stackFrame64, sizeof(stackFrame64));
#ifdef _MSC_VER
  __try
#endif
  {
#ifdef _WIN64
    stackFrame64.AddrPC.Offset = pContext->Rip;
    stackFrame64.AddrPC.Mode = AddrModeFlat;
    stackFrame64.AddrStack.Offset = pContext->Rsp;
    stackFrame64.AddrStack.Mode = AddrModeFlat;
    stackFrame64.AddrFrame.Offset = pContext->Rbp;
    stackFrame64.AddrFrame.Mode = AddrModeFlat;
#else
    stackFrame64.AddrPC.Offset = pContext->Eip;
    stackFrame64.AddrPC.Mode = AddrModeFlat;
    stackFrame64.AddrStack.Offset = pContext->Esp;
    stackFrame64.AddrStack.Mode = AddrModeFlat;
    stackFrame64.AddrFrame.Offset = pContext->Ebp;
    stackFrame64.AddrFrame.Mode = AddrModeFlat;
#endif

    bool successed = true;

    while (successed && (depth < max_depth))
    {
      successed = StackWalk64(
#ifdef _WIN64
        IMAGE_FILE_MACHINE_AMD64,
#else
        IMAGE_FILE_MACHINE_I386,
#endif
        hProcess,
        hThread,
        &stackFrame64,
        pContext,
        NULL,
        SymFunctionTableAccess64,
        SymGetModuleBase64,
        NULL
      ) != FALSE;

      pTrace[depth] = stackFrame64.AddrPC.Offset;

      if (!successed)
      {
        break;
      }

      if (stackFrame64.AddrFrame.Offset == 0)
      {
        break;
      }
      ++depth;
    }
  }
#ifdef _MSC_VER
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }
#endif
  return depth;
}

static size_t check_file_name(char* name)
{
  size_t length = std::strlen(name);
  if (length)
  {
    name[length++] = '.';
    size_t l = std::strlen(name + length);
    length += l ? l : -1;
  }
  return length;
}

static std::list<stack_info> get_stack_list(
  void* bp, void* sp, void* ip,
  size_t max_depth, size_t offset, bool module, bool symbol
)
{
  assert(max_depth <= max_stack_depth());

  QWORD trace[33];
  CONTEXT context;

  ZeroMemory(&context, sizeof(context));
  context.ContextFlags = CONTEXT_FULL;
#ifdef _WIN64
  context.Rbp = (DWORD64)bp;
  context.Rsp = (DWORD64)sp;
  context.Rip = (DWORD64)ip;
#else
  context.Ebp = (DWORD)bp;
  context.Esp = (DWORD)sp;
  context.Eip = (DWORD)ip;
#endif
  size_t depths = stack_walk(trace, max_depth, &context);

  std::list<stack_info> image_list;
  HANDLE hProcess = GetCurrentProcess();
  for (size_t i = offset; i < depths; i++)
  {
    stack_info stack_result;

    {
      DWORD symbolDisplacement = 0;
      IMAGEHLP_LINE64 imageHelpLine;
      imageHelpLine.SizeOfStruct = sizeof(imageHelpLine);

      if (SymGetLineFromAddr64(hProcess, trace[i], &symbolDisplacement, &imageHelpLine))
      {
        stack_result.file_ = std::string(imageHelpLine.FileName, check_file_name(imageHelpLine.FileName));
        std::memset(imageHelpLine.FileName, 0, stack_result.file_.size() + 1);
        stack_result.line_ = (int)imageHelpLine.LineNumber;
      }
      else
      {
        stack_result.line_ = -1;
      }
    }
    if (symbol)
    {
      static const int max_name_length = 1024;
      char symbolBf[sizeof(IMAGEHLP_SYMBOL64) + max_name_length] = { 0 };
      PIMAGEHLP_SYMBOL64 symbol;
      DWORD64 symbolDisplacement64 = 0;

      symbol = (PIMAGEHLP_SYMBOL64)symbolBf;
      symbol->SizeOfStruct = sizeof(symbolBf);
      symbol->MaxNameLength = max_name_length;

      if (SymGetSymFromAddr64(
        hProcess,
        trace[i],
        &symbolDisplacement64,
        symbol)
        )
      {
        stack_result.symbol_ = symbol->Name;
      }
      else
      {
        stack_result.symbol_ = "unknow...";
      }
    }
    if (module)
    {
      IMAGEHLP_MODULE64 imageHelpModule;
      imageHelpModule.SizeOfStruct = sizeof(imageHelpModule);

      if (SymGetModuleInfo64(hProcess, trace[i], &imageHelpModule))
      {
        stack_result.module_ = std::string(imageHelpModule.ImageName, check_file_name(imageHelpModule.ImageName));
        std::memset(imageHelpModule.ImageName, 0, stack_result.module_.size() + 1);
      }
    }
    image_list.push_back(std::move(stack_result));
  }
  return image_list;
}

/// SEH 's filter function.
static int seh_filter(LPEXCEPTION_POINTERS exinfo)
{
  auto ecd = exinfo->ExceptionRecord->ExceptionCode;
  auto stack_depth = max_stack_depth();

#ifdef _WIN64
  auto stack_list =
    get_stack_list(
      (void*)exinfo->ContextRecord->Rbp,
      (void*)exinfo->ContextRecord->Rsp,
      (void*)exinfo->ContextRecord->Rip,
      stack_depth, 0, true, true
      );
#else
  auto stack_list =
    get_stack_list(
      (void*)exinfo->ContextRecord->Ebp,
      (void*)exinfo->ContextRecord->Esp,
      (void*)exinfo->ContextRecord->Eip,
      stack_depth, 0, true, true
      );
#endif

  for (auto& ele : stack_list)
  {
    std::cout << ele << std::endl;
  }
  std::cout << std::endl << std::flush;

  if (ecd == EXCEPTION_ACCESS_VIOLATION)
  {
    //std::cout << "access violation" << std::endl;
    return EXCEPTION_EXECUTE_HANDLER;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}
} /// namespace detail
} /// namespace csegv
