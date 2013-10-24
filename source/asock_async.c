// asock_async.c
//==============================================================================
#ifdef __linux__
# include <malloc.h>
# include <sys/epoll.h>
# include <sys/eventfd.h>
//force clock_gettime w/ CLOCK_MONOTONIC
# define CLOCK_MONOTONIC 1
# define __USE_POSIX199309
# define __USE_POSIX
# define __USE_MISC
# include <time.h>
# include <sys/timerfd.h>
//Force non-gnu strerror_r
# undef __USE_MISC
# include <string.h>
  extern int __xpg_strerror_r (int __errnum, char *__buf, size_t __buflen);
# define strerror_r __xpg_strerror_r
#elif __APPLE__
# include <stdlib.h>
# include <mach/clock.h>
# include <mach/mach.h>
# include <sys/types.h>
# include <sys/event.h>
# include <sys/time.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <clib.h>
#include "asockInternal.h"
//==============================================================================
#ifdef __linux__
# define PLATFORM_INP EPOLLIN
# define PLATFORM_OUT EPOLLOUT
# define PLATFORM_HUP EPOLLHUP|EPOLLRDHUP
#elif  __APPLE__
# define PLATFORM_INP EVFILT_READ
# define PLATFORM_OUT EVFILT_WRITE
# define PLATFORM_HUP EVFILT_HUP
#endif
//==============================================================================
static bool checkBusyLockComplete(asock *sock, asockComplete complete, void *data, i32 connected);
static void queueCompletionProxy(asock *sock, asockComplete complete, void *cdata, u32 status, char *message);
static void requestPollingFor(asock *sock, int flags, threadPoolFunct workOp, u32 timeout);
static bool checkPollingFlags(asock *sock, int desired);
static bool asockPartialFunctionCheck(asock *sock, int status, char *msg, u32 *ms);
static u64  getTimeMilliseconds();
#define UNBUSY(z) pthread_mutex_lock(&(z->mutex)); z->busy=false; pthread_mutex_unlock(&(z->mutex));
//==============================================================================
void asockAcceptWork(asock *sock, bool queue);
void asockAcceptRetry(void *arg);
void asockAcceptComplete(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockConnectWork(asock *sock);
void asockConnectComplete(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockDisconnectWork(asock *sock);
void asockDisconnectComplete(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockReadSomeEnter(void *arg);
void asockReadSomeWork(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockReadEnter(void *arg);
void asockReadWork(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockWriteEnter(void *arg);
void asockWriteWork(void *arg);
//==============================================================================

void asockAccept(asock *sock, asockComplete complete, void *data)
{
  if(checkBusyLockComplete(sock, complete, data, 0) == false) return;
  asockAcceptWork(sock, true);
}

void asockConnect(asock *sock, char *host, u16 port, asockComplete complete, void *data)
{
  if(checkBusyLockComplete(sock, complete, data, -1) == false) return;
  sock->workOpNumber = (u32)port;
  sock->workOpExtra  = (void *)host;
  asockConnectWork(sock);
}

void asockDisconnect(asock *sock, asockComplete complete, void *data)
{
  if(checkBusyLockComplete(sock, complete, data, 1) == false) return;
  asockDisconnectWork(sock);
}

void asockReadSome(asock *sock, void *buff, u32 max, asockComplete complete, void *data)
{
  printf("** asockReadSOME **\n");
  if(checkBusyLockComplete(sock, complete, data, 1) == false) return;
  sock->workOpPartial = 0;
  sock->workOpNumber  = max;
  sock->workOpExtra   = buff;
  sock->workOpTimeout = getTimeMilliseconds() + (u64)sock->timeout;
  threadPoolAddWork(sock->worker->pool, asockReadSomeEnter, sock);
}

void asockRead(asock *sock, void *buff, u32 length, asockComplete complete, void *data)
{
  if(checkBusyLockComplete(sock, complete, data, 1) == false) return;
  sock->workOpPartial = 0;
  sock->workOpNumber  = length;
  sock->workOpExtra   = buff;
  sock->workOpTimeout = getTimeMilliseconds() + (u64)sock->timeout;
  threadPoolAddWork(sock->worker->pool, asockReadEnter, sock);
}

void asockWrite(asock *sock, void *buff, u32 length, asockComplete complete, void *data)
{
  if(checkBusyLockComplete(sock, complete, data, 1) == false) return;
  sock->workOpPartial = 0;
  sock->workOpNumber  = length;
  sock->workOpExtra   = buff;
  sock->workOpTimeout = getTimeMilliseconds() + (u64)sock->timeout;
  threadPoolAddWork(sock->worker->pool, asockWriteEnter, sock);
}

//==============================================================================

static bool checkBusyLockComplete(asock *sock, asockComplete complete, void *data, i32 connected)
{
  if(pthread_mutex_lock(&(sock->mutex)) == 0)
  {
    if(sock->busy == false)
    {
      if((connected ==  1) && (sock->connected == false))
        { pthread_mutex_unlock(&(sock->mutex)); queueCompletionProxy(sock, complete, data, ASOCK_STATUS_DISCONN, clstrdup("Socket not connected")); return false; }
      if((connected == -1) && (sock->connected == true ))
        { pthread_mutex_unlock(&(sock->mutex)); queueCompletionProxy(sock, complete, data, ASOCK_STATUS_CONN, clstrdup("Socket already connected")); return false; }
      sock->busy = true;
      pthread_mutex_unlock(&(sock->mutex));
      sock->complete     = complete;
      sock->completeData = data;
      return true;
    }
    pthread_mutex_unlock(&(sock->mutex));
  }

  queueCompletionProxy(sock, complete, data, ASOCK_STATUS_BUSY, clstrdup("Socket is busy"));
  return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockCompletionProxy(void *arg);
struct asockProxyData
{
  asock        *sock;
  asockComplete complete;
  void         *data;
  u32           status;
  char         *message;
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void queueCompletionProxy(asock *sock, asockComplete complete, void *cdata, u32 status, char *message)
{
  struct asockProxyData *data = (struct asockProxyData *)malloc(sizeof(struct asockProxyData));
  data->sock     = sock;
  data->complete = complete;
  data->data     = cdata;
  data->status   = status;
  data->message  = message;
  threadPoolAddWork(sock->worker->pool, asockCompletionProxy, data);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockCompletionProxy(void *arg)
{
  struct asockProxyData *data = (struct asockProxyData *)arg;
  data->complete(data->sock, data->data, data->status, data->message);
  free(data);
}

//==============================================================================

static void requestPollingFor(asock *sock, int flags, threadPoolFunct workOp, u32 timeout)
{
  sock->workOp = workOp;
#ifdef __linux__
  flags |= EPOLLET;
  struct epoll_event event;
  if(timeout > 0)
  {
    struct itimerspec its; memset(&its, 0, sizeof(struct itimerspec));
    memset(&event, 0, sizeof(struct epoll_event));
    event.events   = EPOLLIN | EPOLLET;
    event.data.ptr = sock;
    epoll_ctl(sock->worker->fdPoll, EPOLL_CTL_ADD, sock->fdTimeout, &event);
    u32 toNS = timeout % 1000;
    u32 toS = (timeout - toNS) / 1000;
    its.it_value.tv_sec  = toS;
    its.it_value.tv_nsec = ((long)toNS) * ((long)1000000); //ms->ns
    timerfd_settime(sock->fdTimeout, 0, &its, NULL);
  }
  memset(&event, 0, sizeof(struct epoll_event));
  event.events   = flags | EPOLLRDHUP; //we always want to know if the other end has dropped
  event.data.ptr = sock;
  epoll_ctl(sock->worker->fdPoll, EPOLL_CTL_ADD, sock->fd, &event);
#elif __APPLE__
  struct kevent changes[2]; int changeCount = 1;
  if(timeout > 0)
  {
    changeCount = 2;
    EV_SET(changes+1, sock->fd, EVFILT_TIMER, EV_ADD|EV_ENABLE|EV_ONESHOT, 0, timeout, sock);
  }
  EV_SET(changes+0, sock->fd, flags, EV_ADD|EV_ENABLE|EV_ONESHOT, 0, 0, sock);
  struct timespec ts; memset(&ts, 0, sizeof(struct timespec));
  kevent(sock->worker->fdPoll, changes, changeCount, NULL, 0, &ts);
#endif
}

static bool checkPollingFlags(asock *sock, int desired)
{
#ifdef __linux__
  if(sock->workOpEvents == EPOLLTIMEOUT)
  {
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_TIMEOUT, clstrdup("Operation timed out"));
    return false;
  }
  //test for our desired flag before testing for hangups.
  //clean disconnects here would prevent us from reading our (presumably) valid data.
  //returning here will allow us to finish this (partial) read, and then we'll get a (read() == 0),
  //indicating a clean disconnect, afterwords.
  if(sock->workOpEvents & desired) return true;

  if((sock->workOpEvents & EPOLLRDHUP) || (sock->workOpEvents & EPOLLHUP))
  {
    sock->connected = false;
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_DISCONN, clstrdup("Connection closed by peer"));
    return false;
  }
  if(sock->workOpEvents & EPOLLERR)
  {
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, clstrdup("Unknown socket epoll error"));
    return false;
  }

  UNBUSY(sock);
  sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, clstrdup("Unhandled socket epoll state"));
  return false;
#elif __APPLE__
  if(sock->workOpEvents == EVFILT_TIMER)
  {
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_TIMEOUT, clstrdup("Operation timed out"));
    return false;
  }

  if(sock->workOpEvents == desired) return true;

  if(sock->workOpEvents == EVFILT_HUP)
  {
    sock->connected = false;
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_DISCONN, clstrdup("Connection closed by peer"));
    return false;
  }

  UNBUSY(sock);
  sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, clstrdup("Unhandled socket kqueue filter"));
  return false;
#endif
}

//==============================================================================

void asockAcceptRetry(void *arg)
{
  asock *sock = (asock *)arg;
  if(checkPollingFlags(sock, PLATFORM_INP) == false) return;
  asockAcceptWork(sock, false);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockAcceptWork(asock *sock, bool queue)
{
  socklen_t socklen = sizeof(struct sockaddr_in);
  struct sockaddr_in *addr = (struct sockaddr_in *)calloc(sizeof(struct sockaddr_in), 1);
  int status = accept(sock->fd, (struct sockaddr *)addr, &socklen);
  if(status == -1)
  {
    free(addr);
    if(errno != EAGAIN)
    {
      char *message, buff[1024];
      message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown accept() error");
      UNBUSY(sock);
      if(queue)
        queueCompletionProxy(sock, sock->complete, sock->completeData, ASOCK_STATUS_FAILED, message);
      else
        sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, message);
      return;
    }
    requestPollingFor(sock, PLATFORM_INP, asockAcceptRetry, sock->timeout);
    return;
  }

  free(addr);
  sock->workOpNumber = (u32)status;
  if(queue == false)
    asockAcceptComplete((void *)sock);
  else
    threadPoolAddWork(sock->worker->pool, asockAcceptComplete, sock);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockAcceptComplete(void *arg)
{
  asock *sock = (asock *)arg;

  asock *clientSock = (asock *)calloc(1,sizeof(asock));
  clientSock->fd        = (int)sock->workOpNumber;
  clientSock->worker    = sock->worker;
  clientSock->timeout   = sock->timeout;
  clientSock->connected = true;
  clientSock->busy      = false;
  pthread_mutex_init(&(clientSock->mutex), NULL);
  fcntl(clientSock->fd, F_SETFL, fcntl(clientSock->fd,F_GETFL,0) | O_NONBLOCK);
#ifdef __linux__
  clientSock->fdTimeout = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
#endif

  asockRegister(clientSock);
  UNBUSY(sock);
  sock->complete(clientSock, sock->completeData, ASOCK_STATUS_OK, NULL);
}

//==============================================================================

void asockConnectWork(asock *sock)
{
  u16 port   = (u16   )sock->workOpNumber;
  char *host = (char *)sock->workOpExtra;

  struct addrinfo hints, *results; memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  int status = getaddrinfo(host, NULL, &hints, &results);
  if(status != 0)
  {
    UNBUSY(sock);
    char *message = clstrdup("Host lookup/resolution failed");
    queueCompletionProxy(sock, sock->complete, sock->completeData, ASOCK_STATUS_BADHOST, message);
    return;
  }
  
  struct sockaddr_in *addr = (struct sockaddr_in *)(results->ai_addr);
  addr->sin_port = htons(port);
  status = connect(sock->fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
  freeaddrinfo(results);

  if(status != 0)
  {
    if(errno == EINPROGRESS)
    {
      requestPollingFor(sock, PLATFORM_OUT, asockConnectComplete, sock->timeout);
      return;
    }
    char *message, buff[1024];
    message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown connect() error");
    UNBUSY(sock);
    queueCompletionProxy(sock, sock->complete, sock->completeData, ASOCK_STATUS_FAILED, message);
    return;
  }

  sock->workOpEvents = PLATFORM_OUT;
  threadPoolAddWork(sock->worker->pool, asockConnectComplete, sock);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockConnectComplete(void *arg)
{
  asock *sock = (asock *)arg;
  if(checkPollingFlags(sock, PLATFORM_OUT) == false) return;

  int result; socklen_t resultLen = sizeof(int);
  if(getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &result, &resultLen) == -1)
  {
    char *message, buff[1024];
    message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown getsockopt() error");
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, message);
    return;
  }
  if(result != 0)
  {
    char *message, buff[1024];
    message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown getsockopt() value error");
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, message);
    return;
  }

  sock->connected = true;
  UNBUSY(sock);
  sock->complete(sock, sock->completeData, ASOCK_STATUS_OK, NULL);
}

//==============================================================================

void asockDisconnectWork(asock *sock)
{
  int status = shutdown(sock->fd, SHUT_RDWR);
  if(status == -1)
  {
    if(errno == ENOTCONN)
    {
      sock->connected = false;
      UNBUSY(sock);
      queueCompletionProxy(sock, sock->complete, sock->completeData, ASOCK_STATUS_OK, NULL);
      return;
    }
    char *message, buff[1024];
    message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown shutdown() error");
    UNBUSY(sock);
    queueCompletionProxy(sock, sock->complete, sock->completeData, ASOCK_STATUS_FAILED, message);
    return;
  }

#ifdef __linux__
  requestPollingFor(sock, EPOLLHUP|EPOLLRDHUP, asockDisconnectComplete, sock->timeout);
#elif __APPLE__
  requestPollingFor(sock, EVFILT_WRITE, asockDisconnectComplete, sock->timeout);
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockDisconnectComplete(void *arg)
{
  asock *sock = (asock *)arg;
  if(checkPollingFlags(sock, PLATFORM_HUP) == false) return;
  sock->connected = false;
  UNBUSY(sock);
  sock->complete(sock, sock->completeData, ASOCK_STATUS_OK, NULL);  
}

//==============================================================================
void asockReadSomeEnter(void *arg)
{
  ((asock *)arg)->workOpEvents = PLATFORM_INP;
  asockReadSomeWork(arg);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockReadSomeWork(void *arg)
{
  asock *sock = (asock *)arg; u32 ms;
  if(asockPartialFunctionCheck(sock, PLATFORM_INP, "ReadSome timed out.", &ms) == false) return;

  //attempt to read some
  ssize_t status = read(sock->fd, (char *)sock->workOpExtra, (size_t)sock->workOpNumber);
  if(status == -1)
  {
    if(errno == EAGAIN)
    {
      requestPollingFor(sock, PLATFORM_INP, asockReadSomeWork, (u32)ms);
      return;
    }
    else
    {
      char *message, buff[1024];
      message = (strerror_r(status, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown read() error");
      UNBUSY(sock);
      sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, message);
      return;
    }
  }
  else if(status == 0)
  {
    sock->connected = false;
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_DISCONN, clstrdup("Connection closed by peer"));
    return;
  }
  else
  {
    sock->workOpNumber  -= (u32)status;
    sock->workOpPartial += (u32)status;
    //got some?
    if(sock->workOpPartial > 0)
    {
      UNBUSY(sock);
      sock->complete(sock, sock->completeData, ASOCK_STATUS_OK, NULL);
      return;
    }
    else
    {
      requestPollingFor(sock, PLATFORM_INP, asockReadSomeWork, (u32)ms);
      return;
    }
  }
}

//==============================================================================
void asockReadEnter(void *arg)
{
  ((asock *)arg)->workOpEvents = PLATFORM_INP;
  asockReadWork(arg);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockReadWork(void *arg)
{
  asock *sock = (asock *)arg; u32 ms;
  if(asockPartialFunctionCheck(sock, PLATFORM_INP, "Read timed out.", &ms) == false) return;

  //attempt to read some
  ssize_t status = read(sock->fd, ((char *)sock->workOpExtra) + sock->workOpPartial, (size_t)sock->workOpNumber);
  if(status == -1)
  {
    if(errno == EAGAIN)
    {
      requestPollingFor(sock, PLATFORM_INP, asockReadWork, (u32)ms);
      return;
    }
    else
    {
      char *message, buff[1024];
      message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown read() error");
      UNBUSY(sock);
      sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, message);
      return;
    }
  }
  else if(status == 0)
  {
    sock->connected = false;
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_DISCONN, clstrdup("Connection closed by peer"));
    return;
  }
  else
  {
    sock->workOpNumber  -= (u32)status;
    sock->workOpPartial += (u32)status;
    //got some?
    if(sock->workOpNumber == 0)
    {
      UNBUSY(sock);
      sock->complete(sock, sock->completeData, ASOCK_STATUS_OK, NULL);
      return;
    }
    else
    {
      requestPollingFor(sock, PLATFORM_INP, asockReadWork, (u32)ms);
      return;
    }
  }
}

//==============================================================================
void asockWriteEnter(void *arg)
{
  ((asock *)arg)->workOpEvents = PLATFORM_OUT;
  asockWriteWork(arg);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void asockWriteWork(void *arg)
{
  asock *sock = (asock *)arg; u32 ms;
  if(asockPartialFunctionCheck(sock, PLATFORM_OUT, "Write timed out.", &ms) == false) return;

  //attempt to write some more
  ssize_t status = write(sock->fd, ((char *)sock->workOpExtra) + sock->workOpPartial, (size_t)sock->workOpNumber);
  if(status == -1)
  {
    if(errno == EAGAIN)
    {
      requestPollingFor(sock, PLATFORM_OUT, asockWriteWork, (u32)ms);
      return;
    }
    else
    {
      char *message, buff[1024];
      message = (strerror_r(errno, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown write() error");
      UNBUSY(sock);
      sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, message);
      return;
    }
  }
  else
  {
    sock->workOpNumber  -= (u32)status;
    sock->workOpPartial += (u32)status;
    //remaining data?
    if(sock->workOpNumber == 0)
    {
      UNBUSY(sock);
      sock->complete(sock, sock->completeData, ASOCK_STATUS_OK, NULL);
      return;
    }
    else
    {
      requestPollingFor(sock, PLATFORM_OUT, asockWriteWork, (u32)ms);
      return;
    }
  }
}

//==============================================================================
static bool asockPartialFunctionCheck(asock *sock, int status, char *msg, u32 *ms)
{
  if(checkPollingFlags(sock, status) == false) return false;

  //remaining data?
  if(sock->workOpNumber == 0)
  {
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_OK, NULL);
    return false;
  }

  //remaining time?
  u64 now = getTimeMilliseconds();
  if( now >= sock->workOpTimeout)
  {
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_TIMEOUT, clstrdup(msg));
    return false;
  }

  //assign remaining time
  *ms = sock->workOpTimeout - now;
  return true;
}
//------------------------------------------------------------------------------
static u64 getTimeMilliseconds()
{
#ifdef __linux__
  struct timespec mt;
  clock_gettime(CLOCK_MONOTONIC, &mt);
#elif __APPLE__
  clock_serv_t cclock;
  mach_timespec_t mt;
  host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
  clock_get_time(cclock, &mt);
  mach_port_deallocate(mach_task_self(), cclock);
#endif
  u64 ret  = (u64)mt.tv_sec  * (u64)1000;
      ret += (u64)mt.tv_nsec / (u64)1000000;
  return ret;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

