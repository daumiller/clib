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
//==============================================================================
#define UNBUSY(z) pthread_mutex_lock(&(z->mutex)); z->busy=false; pthread_mutex_unlock(&(z->mutex));
#define ASOCK_READ_UNTIL_CHUNK_SIZE 4096
//==============================================================================
struct asockReadSomeData
{
  u32  *read;
  void *buff;
};
struct asockReadUntilData
{
  void **receiver;
  void  *until;
  u8     untilLen;
  u32   *readPtr;
  u32    read;
  u32    max;
  u8    *buff;
  u32    buffSize;
  u32    matchOffset;
  u32    partial;
  void  *data;
  asockComplete complete;
};
//==============================================================================
static void asockAcceptWork(asock *sock, bool queue);
static void asockAcceptRetry(void *arg);
static void asockAcceptComplete(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockConnectWork(asock *sock);
static void asockConnectComplete(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockDisconnectWork(asock *sock);
static void asockDisconnectComplete(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockReadSome_noBusy(asock *sock, void *buff, u32 max, u32 *read, asockComplete complete, void *data);
static void asockReadSomeEnter(void *arg);
static void asockReadSomeWork(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockReadEnter(void *arg);
static void asockReadWork(void *arg);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockReadUntil_Helper(asock *sock, struct asockReadUntilData *data, u32 status, char *message);
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockWriteEnter(void *arg);
static void asockWriteWork(void *arg);
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

void asockReadSome(asock *sock, void *buff, u32 max, u32 *read, asockComplete complete, void *data)
{
  if(read) *read = 0;
  if(checkBusyLockComplete(sock, complete, data, 2) == false) return;
  asockReadSome_noBusy(sock, buff, max, read, complete, data);
}
static void asockReadSome_noBusy(asock *sock, void *buff, u32 max, u32 *read, asockComplete complete, void *data)
{
  if(read) *read = 0;
  struct asockReadSomeData *rsd = (struct asockReadSomeData *)malloc(sizeof(struct asockReadSomeData));
  rsd->read = read;
  rsd->buff = buff;

  sock->workOpPartial = 0;
  sock->workOpNumber  = max;
  sock->workOpExtra   = rsd;
  sock->workOpTimeout = (sock->timeout > 0) ? 0 : (getTimeMilliseconds() + (u64)sock->timeout);
  threadPoolAddWork(sock->worker->pool, asockReadSomeEnter, sock);
}

void asockRead(asock *sock, void *buff, u32 length, asockComplete complete, void *data)
{
  if(checkBusyLockComplete(sock, complete, data, 2) == false) return;
  sock->workOpPartial = 0;
  sock->workOpNumber  = length;
  sock->workOpExtra   = buff;
  sock->workOpTimeout = (sock->timeout > 0) ? 0 : (getTimeMilliseconds() + (u64)sock->timeout);
  threadPoolAddWork(sock->worker->pool, asockReadEnter, sock);
}

void asockReadUntil(asock *sock, void **recv, u32 max, u32 *read, void *until, u8 untilLen, asockComplete complete, void *data)
{
  *recv = NULL;
  if(*read) *read = 0;
  struct asockReadUntilData *rud = (struct asockReadUntilData *)malloc(sizeof(struct asockReadUntilData));
  if(checkBusyLockComplete(sock, (asockComplete)asockReadUntil_Helper, (void *)rud, 2) == false)
  {
    free(rud);
    return;
  }

  rud->receiver    = recv;
  rud->until       = until;
  rud->untilLen    = untilLen;
  rud->readPtr     = read;
  rud->read        = 0;
  rud->max         = max;
  rud->buff        = (u8 *)malloc(ASOCK_READ_UNTIL_CHUNK_SIZE);
  rud->buffSize    = ASOCK_READ_UNTIL_CHUNK_SIZE;
  rud->matchOffset = 0;
  rud->partial     = 0;
  rud->complete    = complete;
  rud->data        = data;

  u32 leftToRead = rud->max;
  if(leftToRead > ASOCK_READ_UNTIL_CHUNK_SIZE)
    leftToRead = ASOCK_READ_UNTIL_CHUNK_SIZE;
  asockReadSome_noBusy(sock, rud->buff, leftToRead, &(rud->partial), (asockComplete)asockReadUntil_Helper, (void *)rud);
}

void asockWrite(asock *sock, void *buff, u32 length, asockComplete complete, void *data)
{
  if(checkBusyLockComplete(sock, complete, data, 1) == false) return;
  sock->workOpPartial = 0;
  sock->workOpNumber  = length;
  sock->workOpExtra   = buff;
  sock->workOpTimeout = (sock->timeout > 0) ? 0 : (getTimeMilliseconds() + (u64)sock->timeout);
  threadPoolAddWork(sock->worker->pool, asockWriteEnter, sock);
}

//==============================================================================

static bool checkBusyLockComplete(asock *sock, asockComplete complete, void *data, i32 connected)
{
  if(pthread_mutex_lock(&(sock->mutex)) == 0)
  {
    if(sock->busy == false)
    {
      if((connected ==  2) && (sock->connected == false) && (sock->readBuffLen == 0))
        { pthread_mutex_unlock(&(sock->mutex)); queueCompletionProxy(sock, complete, data, ASOCK_STATUS_DISCONN, clstrdup("Socket not connected")); return false; }
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

static void asockAcceptRetry(void *arg)
{
  asock *sock = (asock *)arg;
  if(checkPollingFlags(sock, PLATFORM_INP) == false) return;
  asockAcceptWork(sock, false);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockAcceptWork(asock *sock, bool queue)
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
static void asockAcceptComplete(void *arg)
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

  //asockRegister(clientSock) {
  pthread_mutex_lock(&(sock->worker->sockCountMutex));
  sock->worker->sockCount++;
  pthread_cond_broadcast(&(sock->worker->sockCountCond));
  pthread_mutex_unlock(&(sock->worker->sockCountMutex)); // }
  UNBUSY(sock);
  sock->complete(clientSock, sock->completeData, ASOCK_STATUS_OK, NULL);
}

//==============================================================================

static void asockConnectWork(asock *sock)
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
static void asockConnectComplete(void *arg)
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

static void asockDisconnectWork(asock *sock)
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
static void asockDisconnectComplete(void *arg)
{
  asock *sock = (asock *)arg;
  if(checkPollingFlags(sock, PLATFORM_HUP) == false) return;
  sock->connected = false;
  UNBUSY(sock);
  sock->complete(sock, sock->completeData, ASOCK_STATUS_OK, NULL);  
}

//==============================================================================
static void asockReadSomeEnter(void *arg)
{
  ((asock *)arg)->workOpEvents = PLATFORM_INP;
  asockReadSomeWork(arg);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockReadSomeWork(void *arg)
{
  asock *sock = (asock *)arg; u32 ms;
  struct asockReadSomeData *rsd = (struct asockReadSomeData *)(sock->workOpExtra);

  if(asockPartialFunctionCheck(sock, PLATFORM_INP, "ReadSome timed out.", &ms) == false)
  {
    free(rsd);
    return;
  }

  ssize_t status;
  //see if we already have some data (left over from an asockReadUntil)
  if(sock->readBuffLen > 0)
  {
    u32 taken = sock->readBuffLen;

    //simple case, we can take it all
    if(taken <= sock->workOpNumber)
    {
      memcpy(rsd->buff, sock->readBuff, taken);
      free(sock->readBuff);
      sock->readBuff = NULL;
      sock->readBuffLen = 0;
    }
    //PITA case, we can only take part of the readBuff
    else
    {
      taken = sock->workOpNumber;
      memcpy(rsd->buff, sock->readBuff, taken);
      sock->readBuffLen -= taken;
      u8 *newBuff = (u8 *)malloc(sock->readBuffLen);
      memcpy(newBuff, sock->readBuff + taken, sock->readBuffLen);
      free(sock->readBuff);
      sock->readBuff = newBuff;
    }

    status = (ssize_t)taken;
  }
  //else, attempt to read some
  else
    status = read(sock->fd, (char *)rsd->buff, (size_t)sock->workOpNumber);

  if(status == -1)
  {
    if(errno == EAGAIN)
    {
      requestPollingFor(sock, PLATFORM_INP, asockReadSomeWork, (u32)ms);
      return;
    }
    else
    {
      free(rsd);
      char *message, buff[1024];
      message = (strerror_r(status, buff, 1024) == 0) ? clstrdup(buff) : clstrdup("Unknown read() error");
      UNBUSY(sock);
      sock->complete(sock, sock->completeData, ASOCK_STATUS_FAILED, message);
      return;
    }
  }
  else if(status == 0)
  {
    free(rsd);
    sock->connected = false;
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_DISCONN, clstrdup("Connection closed by peer"));
    return;
  }
  else
  {
    if(rsd->read)
      *(rsd->read) = (u32)status;
    free(rsd);
    UNBUSY(sock);
    sock->complete(sock, sock->completeData, ASOCK_STATUS_OK, NULL);
    return;
  }
}

//==============================================================================
static void asockReadEnter(void *arg)
{
  ((asock *)arg)->workOpEvents = PLATFORM_INP;
  asockReadWork(arg);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockReadWork(void *arg)
{
  asock *sock = (asock *)arg; u32 ms;
  if(asockPartialFunctionCheck(sock, PLATFORM_INP, "Read timed out.", &ms) == false) return;

  ssize_t status;
  //see if we already have some data (left over from an asockReadUntil)
  if(sock->readBuffLen > 0)
  {
    u32 taken = sock->readBuffLen;

    //simple case, we can take it all
    if(taken <= sock->workOpNumber)
    {
      memcpy(sock->workOpExtra, sock->readBuff, taken);
      free(sock->readBuff);
      sock->readBuff = NULL;
      sock->readBuffLen = 0;
    }
    //PITA case, we can only take part of the readBuff
    else
    {
      taken = sock->workOpNumber;
      memcpy(sock->workOpExtra, sock->readBuff, taken);
      sock->readBuffLen -= taken;
      u8 *newBuff = (u8 *)malloc(sock->readBuffLen);
      memcpy(newBuff, sock->readBuff + taken, sock->readBuffLen);
      free(sock->readBuff);
      sock->readBuff = newBuff;
    }

    status = (ssize_t)taken;
  }
  //else, attempt to read some
  else
    status = read(sock->fd, (char *)sock->workOpExtra, (size_t)sock->workOpNumber);

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
static void asockReadUntil_Helper(asock *sock, struct asockReadUntilData *rud, u32 status, char *message)
{
  // read until disconnect
  if((status == ASOCK_STATUS_DISCONN) && (rud->untilLen == 0))
  {
    if(rud->readPtr) *(rud->readPtr) = rud->read;
    *(rud->receiver) = realloc(rud->buff, rud->read);
    asockComplete complete = rud->complete;
    void *data = rud->data;
    free(rud);
    complete(sock, data, ASOCK_STATUS_OK, NULL);
    return;
  }

  // errors...
  if((status != ASOCK_STATUS_OKAY) && (status != ASOCK_STATUS_DISCONN))
  {
    asockComplete complete = rud->complete;
    void *data = rud->data;
    free(rud->buff);
    free(rud);
    complete((void *)sock, data, status, message);
    return;
  }
  if((status == ASOCK_STATUS_DISCONN) && (rud->partial == 0))
  {
    asockComplete complete = rud->complete;
    void *data = rud->data;
    free(rud->buff);
    free(rud);
    complete(sock, data, ASOCK_STATUS_HITMAX, clstrdup("didn't find ReadUntil token within expected length"));
    return;
  }

  // we have some data to process
  rud->read += rud->partial;

  // simple case : we're reading until disconnect
  if(rud->untilLen == 0)
  {
    rud->buffSize = rud->read + ASOCK_READ_UNTIL_CHUNK_SIZE;
    rud->buff     = realloc(rud->buff, rud->buffSize);
    asockReadSome_noBusy(sock, rud->buff, ASOCK_READ_UNTIL_CHUNK_SIZE, &(rud->partial), (asockComplete)asockReadUntil_Helper, (void *)rud);
    return;
  }

  // simple-ish case : not enough data to compare
  if((rud->read - rud->matchOffset) < rud->untilLen)
  {
    rud->buffSize = rud->read + ASOCK_READ_UNTIL_CHUNK_SIZE;
    rud->buff     = realloc(rud->buff, rud->buffSize);
    if(rud->read < rud->max)
      asockReadSome_noBusy(sock, rud->buff, ASOCK_READ_UNTIL_CHUNK_SIZE, &(rud->partial), (asockComplete)asockReadUntil_Helper, (void *)rud);
    else
    {
      if(rud->readPtr) *(rud->readPtr) = rud->read;
      *(rud->receiver) = realloc(rud->buff, rud->read);
      asockComplete complete = rud->complete;
      void *data = rud->data;
      free(rud);
      complete(sock, data, ASOCK_STATUS_HITMAX, clstrdup("didn't find ReadUntil token within expected length"));
    }
    return;
  }

  // we have to scan for a match...
  u8 matched;
  for(u32 a,b,i=rud->matchOffset; i<rud->read; i++)
  {
    a=i; b=0; matched=0;
    while((a<rud->read) && (rud->buff[a] == ((u8 *)rud->until)[b]))
    {
      matched++;
      // we actually found the damn thing
      if(matched == rud->untilLen)
      {
        // store remaining data in our partial buffer
        if((a+1) < rud->read) //partial data
        {
          u32 partial = rud->read - (a+1);
          sock->readBuff = (u8 *)realloc(sock->readBuff, sock->readBuffLen + partial);
          memcpy(sock->readBuff + sock->readBuffLen, rud->buff+a+1, partial);
          sock->readBuffLen += partial;
        }
        // return up-to, and including, our "until" token
        if(rud->readPtr) *(rud->readPtr) = rud->read;
        *(rud->receiver) = realloc(rud->buff, rud->read);
        asockComplete complete = rud->complete;
        void *data = rud->data;
        free(rud);
        complete(sock, data, ASOCK_STATUS_OK, NULL);
        return;
      }
      a++; b++;
    }
    // store our matchOffset so we're not scanning the whole buffer every readSome()
    rud->matchOffset = i;
    if(a == rud->read)
    {
      // this check is important:
      // if we have a partial match running up until the end of our data,
      // bail early so we don't increment the matchOffset variable passed the beginning of the partial
      break;
    }
  }
  // we didn't find a match...

  // have we read as much as we can?
  if(rud->buffSize == rud->max)
  {
    rud->buffSize = rud->read + ASOCK_READ_UNTIL_CHUNK_SIZE;
    rud->buff     = realloc(rud->buff, rud->buffSize);
    if(rud->readPtr) *(rud->readPtr) = rud->read;
    *(rud->receiver) = realloc(rud->buff, rud->read);
    asockComplete complete = rud->complete;
    void *data = rud->data;
    free(rud);
    complete(sock, data, ASOCK_STATUS_HITMAX, clstrdup("didn't find ReadUntil token within expected length"));
    return;
  }

  // more to read then
  u32 leftToRead = rud->max - rud->buffSize;
  if(leftToRead > ASOCK_READ_UNTIL_CHUNK_SIZE)
    leftToRead = ASOCK_READ_UNTIL_CHUNK_SIZE;
  rud->buffSize += leftToRead;
  rud->buff = (u8 *)realloc(rud->buff, rud->buffSize);
  asockReadSome_noBusy(sock, rud->buff + rud->read, leftToRead, &(rud->partial), (asockComplete)asockReadUntil_Helper, (void *)rud);
}

//==============================================================================
static void asockWriteEnter(void *arg)
{
  ((asock *)arg)->workOpEvents = PLATFORM_OUT;
  asockWriteWork(arg);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void asockWriteWork(void *arg)
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
  if(sock->workOpTimeout > 0)
  {
    u64 now = getTimeMilliseconds();
    if( now >= sock->workOpTimeout)
    {
      UNBUSY(sock);
      sock->complete(sock, sock->completeData, ASOCK_STATUS_TIMEOUT, clstrdup(msg));
      return false;
    }

    //assign remaining time
    *ms = sock->workOpTimeout - now;
  }
  else
    *ms = 0;

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

