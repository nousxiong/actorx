//
// Test hook win32 iocp.
//

#include <actorx/utest/all.hpp>


#ifdef _MSC_VER

#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0700
#endif
#define ASIO_STANDALONE
#include <asio.hpp>

#include <WinSock2.h>
#include <xhook.hpp>

#include <actorx/asev/all.hpp>

#include <set>
#include <map>
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <memory>

#define MAX_BUFFER_LEN 1024 * 8

// All api will be hooked, must declare with WINAPI, bcz:
// http://2309998.blog.51cto.com/2299998/1273090
typedef SOCKET(WSAAPI *socket_t)(
  _In_ int af,
  _In_ int type,
  _In_ int protocol
  );
static socket_t socket_f = (socket_t)&::socket;

typedef HANDLE(WINAPI *CreateIoCompletionPortT)(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE ExistingCompletionPort,
  _In_ ULONG_PTR CompletionKey,
  _In_ DWORD NumberOfConcurrentThreads
  );
static CreateIoCompletionPortT CreateIoCompletionPortF = (CreateIoCompletionPortT)&::CreateIoCompletionPort;
//static CreateIoCompletionPortT CreateIoCompletionPortF = (CreateIoCompletionPortT)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateIoCompletionPort");

// Hook CloseHandle.
typedef BOOL(WINAPI *CloseHandleT)(_In_ HANDLE hObject);
static CloseHandleT CloseHandleF = (CloseHandleT)&::CloseHandle;

// Our CloseHandle.
#define RELEASE_HANDLE(x) {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandleF(x);x = NULL;}}

// Hook accept.
typedef SOCKET(WSAAPI *accept_t)(
  _In_    SOCKET          s,
  _Out_   struct sockaddr *addr,
  _Inout_ int             *addrlen
  );
static accept_t accept_f = (accept_t)&::accept;

// Hook WSASocketW
typedef SOCKET(WSAAPI *WSASocketWT)(
  _In_ int af,
  _In_ int type,
  _In_ int protocol,
  _In_opt_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _In_ GROUP g,
  _In_ DWORD dwFlags
  );
static WSASocketWT WSASocketWF = (WSASocketWT)&::WSASocketW;

// Hook WSARecv.
typedef int(WSAAPI *WSARecvT)(
  _In_    SOCKET                             s,
  _Inout_ LPWSABUF                           lpBuffers,
  _In_    DWORD                              dwBufferCount,
  _Out_   LPDWORD                            lpNumberOfBytesRecvd,
  _Inout_ LPDWORD                            lpFlags,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
  );
static WSARecvT WSARecvF = (WSARecvT)&::WSARecv;

// Hook WSASend.
typedef int(WSAAPI *WSASendT)(
  _In_  SOCKET                             s,
  _In_  LPWSABUF                           lpBuffers,
  _In_  DWORD                              dwBufferCount,
  _Out_ LPDWORD                            lpNumberOfBytesSent,
  _In_  DWORD                              dwFlags,
  _In_  LPWSAOVERLAPPED                    lpOverlapped,
  _In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
  );
static WSASendT WSASendF = (WSASendT)&::WSASend;

namespace usr
{
using logger_ptr = std::shared_ptr<spdlog::logger>;

enum opt_type
{
  opt_accept,
  opt_send,
  opt_recv,
  opt_null,
};

static void close_socket(SOCKET& skt)
{
  if (skt != INVALID_SOCKET)
  {
    ::closesocket(skt);
    skt = INVALID_SOCKET;
  }
}

struct io_context
{
  io_context()
    : skt_(INVALID_SOCKET)
    , opt_(opt_null)
    , corctx_(nullptr)
  {
    ZeroMemory(&overlapped_, sizeof(overlapped_));
    wsa_buf_.buf = buffer_;
    wsa_buf_.len = MAX_BUFFER_LEN;
  }

  ~io_context()
  {
    close_socket(skt_);
  }

  OVERLAPPED overlapped_;
  SOCKET skt_;
  WSABUF wsa_buf_;
  char buffer_[MAX_BUFFER_LEN];
  opt_type opt_;
  std::error_code ec_;

  // May be null.
  asev::corctx_t* corctx_;
};

struct iocp_service
{
  explicit iocp_service(logger_ptr logger, HANDLE iocp, size_t thread_num)
    : logger_(logger)
    , iocp_(iocp)
    , thread_num_(set_thread_num(thread_num))
  {
    logger_->info("iocp thread num: {}", thread_num_);
  }

  static size_t set_thread_num(size_t thread_num)
  {
    if (thread_num == (std::numeric_limits<DWORD>::max)())
    {
      thread_num = std::thread::hardware_concurrency() / 2;
    }

    if (thread_num == 0)
    {
      thread_num = 1;
    }
    //return thread_num;
    return 1;
  }

  void start()
  {
    for (size_t i = 0; i < thread_num_; ++i)
    {
      workers_.push_back(
        std::thread(
          [this]()
          {
            DWORD dwBytesTransfered = 0;
            OVERLAPPED_ENTRY olarr;
            ULONG num = 0;

            while (true)
            {
              ZeroMemory((char*)&olarr, sizeof(OVERLAPPED_ENTRY));
              BOOL bReturn =
                ::GetQueuedCompletionStatusEx(
                  iocp_,
                  &olarr,
                  1,
                  &num,
                  INFINITE,
                  FALSE
                );

              /*if ((DWORD)olarr.lpCompletionKey == 0)
              {
              break;
              }*/

              if (!bReturn)
              {
                DWORD dwErr = GetLastError();
                logger_->error("iocp {} GetQueuedCompletionStatus error: {}", iocp_, dwErr);
                if (dwErr == ERROR_INVALID_HANDLE)
                {
                  break;
                }
                continue;
              }

              dwBytesTransfered = olarr.dwNumberOfBytesTransferred;
              io_context* ioctx = CONTAINING_RECORD(olarr.lpOverlapped, io_context, overlapped_);
              if ((0 == dwBytesTransfered) && (opt_recv == ioctx->opt_ || opt_send == ioctx->opt_))
              {
                logger_->error("client disconnect.");
                continue;
              }

              switch (ioctx->opt_)
              {
              case opt_accept:
              {
                ioctx->ec_.clear();
                if (ioctx->corctx_ != nullptr)
                {
                  ioctx->corctx_->resume();
                }
              }break;
              case opt_null:
              {
                return;
              }break;
              }
            }
          })
        );
    }
  }

  void stop()
  {
    ioctx_.opt_ = opt_null;
    for (size_t i = 0; i < workers_.size(); ++i)
    {
      PostQueuedCompletionStatus(iocp_, 0, NULL, &ioctx_.overlapped_);
    }

    for (auto& worker : workers_)
    {
      worker.join();
    }
  }

  logger_ptr logger_;
  HANDLE iocp_;
  size_t const thread_num_;
  io_context ioctx_;
  std::vector<std::thread> workers_;
};

struct thr_affix : public asev::affix_base
{
  thr_affix()
    : asev::affix_base(this)
    , is_socket_(false)
  {
  }

  bool is_socket_;
  std::map<HANDLE, iocp_service> iocp_svcs_;
};

struct cor_affix : public asev::affix_base
{
  explicit cor_affix(asev::corctx_t& self)
    : asev::affix_base(this)
  {
    ioctx_.corctx_ = &self;
    self.add_affix(this);
  }

  io_context ioctx_;
};

static SOCKET WSAAPI socket_h(int af, int type, int protocol)
{
  auto thrctx = asev::ev_service::current();
  if (thrctx == nullptr)
  {
    return socket_f(af, type, protocol);
  }
  return WSASocketW(af, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
}

// Note: our hook function MUST be WINAPI/WSAAPI bcz __stdcall is part of declare when use winapis.
// http://2309998.blog.51cto.com/2299998/1273090
static HANDLE WINAPI CreateIoCompletionPortH(
  HANDLE FileHandle,
  HANDLE ExistingCompletionPort,
  ULONG_PTR CompletionKey,
  DWORD NumberOfConcurrentThreads
)
{
  auto rt =
    CreateIoCompletionPortF(
      FileHandle,
      ExistingCompletionPort,
      CompletionKey,
      NumberOfConcurrentThreads
      );

  if (rt == NULL)
  {
    return rt;
  }

  auto thrctx = asev::ev_service::current();
  if (thrctx == nullptr)
  {
    // This means not in asev's threads, so we ignore it.
    return rt;
  }

  auto thraf = thrctx->get_affix<thr_affix>();
  ACTORX_ASSERTS(thraf != nullptr);

  if (!thraf->is_socket_ && ExistingCompletionPort == NULL)
  {
    // New iocp create, save it at thrctx's affix.
    auto pr = thraf->iocp_svcs_.emplace(
      rt, iocp_service(thrctx->get_ev_service().get_logger(), rt, NumberOfConcurrentThreads)
      );
    ACTORX_ASSERTS(pr.second);

    // Start this new iocp_service.
    pr.first->second.start();
    
    /*auto corctx = thrctx->get_corctx();
    ACTORX_ASSERTS(corctx != nullptr);

    auto coraf = corctx->get_affix<cor_affix>();
    ACTORX_ASSERTS(coraf != nullptr);

    if (coraf->iocp_ != NULL)
    {
      return rt;
    }

    coraf->iocp_ = rt;*/
  }

  return rt;
}

static BOOL WINAPI CloseHandleH(HANDLE hObject)
{
  auto thrctx = asev::ev_service::current();
  if (thrctx == nullptr)
  {
    // This means not in asev's threads, so we ignore it.
    return CloseHandleF(hObject);
  }

  auto thraf = thrctx->get_affix<thr_affix>();
  if (thraf == nullptr)
  {
    return CloseHandleF(hObject);
  }

  auto itr = thraf->iocp_svcs_.find(hObject);
  if (itr != thraf->iocp_svcs_.end())
  {
    // Find our iocp handle, stop its thread_pool and erase it.
    itr->second.stop();
    thraf->iocp_svcs_.erase(itr);
    return CloseHandleF(hObject);
  }
  else
  {
    return CloseHandleF(hObject);
  }
}

static SOCKET WSAAPI accept_h(SOCKET s, struct sockaddr* addr, int* addrlen)
{
  auto thrctx = asev::ev_service::current();
  if (thrctx == nullptr)
  {
    return accept_f(s, addr, addrlen);
  }

  auto corctx = thrctx->get_corctx();
  ACTORX_EXPECTS(corctx != nullptr);

  auto coraf = corctx->get_affix<cor_affix>();
  ACTORX_EXPECTS(coraf != nullptr);

  io_context& ioctx = coraf->ioctx_;
  DWORD bytes = 0;
  WSABUF* wbuf = &ioctx.wsa_buf_;
  LPOVERLAPPED ol = &ioctx.overlapped_;
  DWORD addr_len = sizeof(SOCKADDR_IN) + 16;
  ioctx.ec_.clear();
  ioctx.opt_ = opt_accept;

  // Make socket.
  auto skt = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (skt == INVALID_SOCKET)
  {
    return INVALID_SOCKET;
  }

  // AcceptEx.
  auto result = ::AcceptEx(s, skt, wbuf->buf, 0, addr_len, addr_len, &bytes, ol);
  // Check AcceptEx result.
  DWORD last_error = ::WSAGetLastError();
  if (!result && last_error != WSA_IO_PENDING)
  {
    close_socket(skt);
    return INVALID_SOCKET;
  }
  else
  {
    corctx->yield();
  }

  if (ioctx.ec_)
  {
    close_socket(skt);
  }

  ioctx.skt_ = skt;
  return skt;
}

static SOCKET WSAAPI WSASocketWH(
  _In_ int af,
  _In_ int type,
  _In_ int protocol,
  _In_opt_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _In_ GROUP g,
  _In_ DWORD dwFlags
  )
{
  auto thrctx = asev::ev_service::current();
  if (thrctx == nullptr)
  {
    return WSASocketWF(af, type, protocol, lpProtocolInfo, g, dwFlags);
  }

  auto thraf = thrctx->get_affix<thr_affix>();
  ACTORX_ASSERTS(thraf != nullptr);

  thraf->is_socket_ = true;
  auto s = WSASocketWF(af, type, protocol, lpProtocolInfo, g, dwFlags);
  thraf->is_socket_ = false;
  return s;
}

static int WSAAPI WSARecvH(
  _In_    SOCKET                             s,
  _Inout_ LPWSABUF                           lpBuffers,
  _In_    DWORD                              dwBufferCount,
  _Out_   LPDWORD                            lpNumberOfBytesRecvd,
  _Inout_ LPDWORD                            lpFlags,
  _In_    LPWSAOVERLAPPED                    lpOverlapped,
  _In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
  return -1;
}

} // namespace usr

UTEST_CASE(test_hook_iocp)
{
  //asio::io_service ios;
  asev::ev_service evs(1);
  //asev::ev_service evs;
  evs.tstart(
    [](asev::thrctx_t& thrctx)
    {
      XHookRestoreAfterWith();
      XHookTransactionBegin();
      XHookUpdateThread(GetCurrentThread());
      XHookAttach((PVOID*)&accept_f, usr::accept_h);
      XHookAttach((PVOID*)&socket_f, usr::socket_h);
      XHookAttach((PVOID*)&WSARecvF, usr::WSARecvH);
      XHookAttach((PVOID*)&CreateIoCompletionPortF, usr::CreateIoCompletionPortH);
      XHookAttach((PVOID*)&WSASocketWF, usr::WSASocketWH);
      XHookAttach((PVOID*)&CloseHandleF, usr::CloseHandleH);
      XHookTransactionCommit();

      // Set thr_affix.
      auto thraf = new usr::thr_affix;
      thrctx.add_affix(thraf);
    });

  evs.texit(
    [](asev::thrctx_t& thrctx)
    {
      // Clear thr_affix.
      auto thraf = thrctx.get_affix<usr::thr_affix>();
      for (auto& iocpsvc : thraf->iocp_svcs_)
      {
        iocpsvc.second.stop();
        CloseHandleF(iocpsvc.second.iocp_);
      }

      auto affixes = thrctx.clear_affix();
      for (auto affix_ptr : affixes)
      {
        delete affix_ptr;
      }
    });

  auto logger = evs.get_logger();
  asev::strand_t snd(evs);
  std::atomic_size_t count(1);
  evs.spawn(
    [&](asev::corctx_t& self)
    {
      logger->info("Begin corctx accept!");
      asio::io_service acpr_ios;
      //auto iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
      usr::cor_affix coraf(self);

      //auto s = ::socket(AF_INET, SOCK_STREAM, 0);

      asio::ip::tcp::endpoint ep(asio::ip::tcp::v4(), 23333);
      asio::ip::tcp::acceptor acpr(acpr_ios, ep);

      //asio::io_service skt_ios;
      asio::ip::tcp::socket skt(acpr_ios);
      auto bt = std::chrono::system_clock::now();
      acpr.accept(skt);
      auto eclipse =
        std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now() - bt
          );
      logger->info("End corctx accept, eclipse: {}", eclipse.count());

      if (--count == 0)
      {
        evs.stop();
      }
    });

  evs.run();

  logger->info("done.");
}

#endif // _MSC_VER
