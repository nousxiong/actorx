//
// posix.hpp
//

#pragma once


#include "actorx/coctx/config.hpp"
#include "actorx/coctx/stack_size.hpp"

#include <thread>
#include <system_error>

#include <cstdlib>
#include <cstring>
#include <unistd.h>


#if _POSIX_MAPPED_FILES
# include <sys/mman.h>
# define COCTX_MMAP 1
# ifndef MAP_ANONYMOUS
#   ifdef MAP_ANON
#     define MAP_ANONYMOUS MAP_ANON
#   else
#     undef COCTX_MMAP
#   endif
# endif
# include <limits.h>
#else
# undef COCTX_MMAP
#endif

#if _POSIX_MEMORY_PROTECTION
# ifndef COCTX_GUARDPAGES
#   define COCTX_GUARDPAGES 4
# endif
#else
# undef COCTX_GUARDPAGES
#endif

#if !COCTX_MMAP
# undef COCTX_GUARDPAGES
#endif

#if !__i386 && !__x86_64 && !__powerpc && !__m68k && !__alpha && !__mips && !__sparc64
# undef COCTX_GUARDPAGES
#endif

#ifndef COCTX_GUARDPAGES
# define COCTX_GUARDPAGES 0
#endif

#if !COCTX_PAGESIZE
# if !COCTX_MMAP
#   define COCTX_PAGESIZE 4096
# else
static size_t coctx_pagesize()
{
  static thread_local size_t pagesize = 0;
  if (pagesize == 0)
  {
    pagesize = sysconf(_SC_PAGESIZE);
  }
  return pagesize;
}
#   define COCTX_PAGESIZE coctx_pagesize()
# endif
#endif


// Context's func.
typedef void (*coctx_func)(void*);

// coctx's asm context.
struct coctx_asmctx
{
  coctx_asmctx()
    : sp_(nullptr)
  {
  }

  // Must be at offset 0.
  void** sp_;
};

extern "C"
{
//! coctx_transfer
/**
 * The following prototype defines the coroutine switching function.
 * This function is thread-safe and reentrant.
 */
void __attribute__ ((__noinline__, __regparm__(2)))
  coctx_transfer(coctx_asmctx* prev, coctx_asmctx* next);
}

asm(
  "\t.text\n"
  // Can't use .globl bcz multi modules will redefine coctx_transfer.
  "\t.extern coctx_transfer\n"
  "coctx_transfer:\n"
#if __amd64
# define COCTX_NUM_SAVED 6
  "\tpushq %rbp\n"
  "\tpushq %rbx\n"
  "\tpushq %r12\n"
  "\tpushq %r13\n"
  "\tpushq %r14\n"
  "\tpushq %r15\n"
  "\tmovq %rsp, (%rdi)\n"
  "\tmovq (%rsi), %rsp\n"
  "\tpopq %r15\n"
  "\tpopq %r14\n"
  "\tpopq %r13\n"
  "\tpopq %r12\n"
  "\tpopq %rbx\n"
  "\tpopq %rbp\n"
  "\tpopq %rcx\n"
  "\tjmpq *%rcx\n"
#elif __i386
# define COCTX_NUM_SAVED 4
  "\tpushl %ebp\n"
  "\tpushl %ebx\n"
  "\tpushl %esi\n"
  "\tpushl %edi\n"
  "\tmovl %esp, (%eax)\n"
  "\tmovl (%edx), %esp\n"
  "\tpopl %edi\n"
  "\tpopl %esi\n"
  "\tpopl %ebx\n"
  "\tpopl %ebp\n"
  "\tpopl %ecx\n"
  "\tjmpl *%ecx\n"
#else
# error unsupported architecture
#endif
);


namespace coctx
{
namespace detail
{
// Context's stack.
class stack
{
  stack(stack const&) = delete;
  stack& operator=(stack const&) = delete;
  stack(stack&&) = delete;
  stack& operator=(stack&&) = delete;

public:
  stack()
    : sptr_(nullptr)
    , size_(0)
  {
  }

  explicit stack(size_t size)
    : sptr_(nullptr)
    , size_(0)
  {
    auto const pagesize = COCTX_PAGESIZE;
    size_ = (size * sizeof(void*) + pagesize - 1) / pagesize * pagesize;
    auto ssze = size_ + COCTX_GUARDPAGES * pagesize;

    gsl::owner<void*> sptr = nullptr;
    auto err = std::make_error_code(std::errc::not_enough_memory);

#if COCTX_MMAP
    // mmap supposedly does allocate-on-write for us.
    sptr = mmap(0, ssze, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (sptr == (void*)-1)
    {
      // Some systems don't let us have executable heap.
      // We assume they won't need executable stack in that case.
      sptr = mmap(0, ssze, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

      if (sptr == (void*)-1)
      {
        throw std::system_error(err, "stack mmap alloc failed!");
      }
    }

# if CORO_GUARDPAGES
    mprotect(sptr, COCTX_GUARDPAGES * pagesize, PROT_NONE);
# endif

    sptr = (void*)((char*)sptr + COCTX_GUARDPAGES * pagesize);
#else
    sptr = std::malloc(ssze);
    if (sptr == nullptr)
    {
      throw std::system_error(err, "stack malloc failed!");
    }
#endif

    sptr_ = sptr;
  }

  ~stack()
  {
    if (sptr_ != nullptr)
    {
#if COCTX_MMAP
      auto const pagesize = COCTX_PAGESIZE;
      munmap((void*)((char *)sptr_ - COCTX_GUARDPAGES * pagesize), size_ + COCTX_GUARDPAGES * pagesize);
#else
      std::free(sptr_);
#endif
    }
    sptr_ = nullptr;
  }

public:
  void* get_sptr() noexcept
  {
    return sptr_;
  }

  size_t size() const noexcept
  {
    return size_;
  }

private:
  gsl::owner<void*> sptr_;
  size_t size_;
};

// For asm init data.
struct asm_init
{
  static asm_init* current() noexcept
  {
    return current(asm_init{1});
  }

  static asm_init* current(asm_init&& ai) noexcept
  {
    // Local asm_init.
    static thread_local asm_init local_ai{1};
    if (ai.null_ == 0)
    {
      local_ai = std::move(ai);
    }
    return &local_ai;
  }

  int null_;
  coctx_asmctx ctx_;
  coctx_asmctx cctx_;
  coctx_func f_;
  void* arg_;
};

static void init_coctx()
{
  gsl::not_null<asm_init*> ai = asm_init::current();
  ACTORX_ASSERTS(ai->null_ == 0);

  auto& ctx = ai->ctx_;
  auto& cctx = ai->cctx_;
  volatile auto f = ai->f_;
  volatile auto arg = ai->arg_;

  coctx_transfer(&ctx, &cctx);

#if __GCC_HAVE_DWARF2_CFI_ASM && __amd64
  asm(".cfi_undefined rip");
#endif

  f((void*)arg);

  // The new coro returned. bad. just abort() for now.
  ACTORX_ENSURES(false);
}

//! Make coctx_asmctx.
/**
 * @param stk stack struck.
 * @param f coctx function.
 * @param arg coctx func's arg.
 * @return coctx_asmctx.
 */
static coctx_asmctx make_coctx(stack& stk, coctx_func f, void* arg)
{
  // Init asm_init.
  auto ai = asm_init::current(asm_init{0});
  ai->f_ = f;
  ai->arg_ = arg;

  auto sptr = stk.get_sptr();
  auto ssize = stk.size();

  ai->ctx_.sp_ = (void**)(ssize + (char*)sptr);
  *--ai->ctx_.sp_ = (void*)std::abort; // Needed for alignment only.
  *--ai->ctx_.sp_ = (void*)init_coctx;
  ai->ctx_.sp_ -= COCTX_NUM_SAVED;
  std::memset(ai->ctx_.sp_, 0, sizeof(*ai->ctx_.sp_) * COCTX_NUM_SAVED);

  coctx_transfer(&ai->cctx_, &ai->ctx_);
  return ai->ctx_;
}
} // namespace detail
} // namespace coctx
