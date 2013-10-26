// asock_sync.c
//==============================================================================
#ifdef __linux__
# include <malloc.h>
# include <sys/epoll.h>
# include <sys/eventfd.h>
//i hate you, POSIX/GNU/XOPENSOURCE header define crap...
# define CLOCK_MONOTONIC 1
# include <time.h>
# include <sys/timerfd.h>
# define __USE_POSIX
# define __USE_MISC
#elif __APPLE__
# include <stdlib.h>
#endif
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <clib/string.h>
#include <clib/asock.h>
#include "asockInternal.h"
//==============================================================================
static void asockRegister(asock *sock);
static void asockUnregister(asock *sock);

asock *asockCreate(asockWorker *worker, u32 timeout)
{
  if(worker == NULL) return NULL;

  asock *sock = (asock *)calloc(sizeof(asock), 1);
  sock->fd          = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  sock->timeout     = timeout;
  sock->readBuff    = NULL;
  sock->readBuffLen = 0;
  sock->worker      = worker;
  sock->connected   = false;
  sock->busy        = false;
  pthread_mutex_init(&(sock->mutex), NULL);
  fcntl(sock->fd, F_SETFL, fcntl(sock->fd,F_GETFL,0) | O_NONBLOCK);

#ifdef __linux__
  sock->fdTimeout = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
#endif

  asockRegister(sock);
  return sock;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void asockFree(asock **sock)
{
  close((*sock)->fd);

#ifdef __linux__
  close((*sock)->fdTimeout);
#endif

  asockUnregister(*sock);
  free(*sock);
  *sock = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void asockRegister(asock *sock)
{
  pthread_mutex_lock(&(sock->worker->sockCountMutex));
  sock->worker->sockCount++;
  pthread_cond_broadcast(&(sock->worker->sockCountCond));
  pthread_mutex_unlock(&(sock->worker->sockCountMutex));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void asockUnregister(asock *sock)
{
  pthread_mutex_lock(&(sock->worker->sockCountMutex));
  sock->worker->sockCount--;
  pthread_cond_broadcast(&(sock->worker->sockCountCond));
  pthread_mutex_unlock(&(sock->worker->sockCountMutex));
}

//------------------------------------------------------------------------------

bool asockIsBusy(asock *sock)
{
  if(pthread_mutex_lock(&(sock->mutex)) != 0) return false;
  bool ret = sock->busy;
  pthread_mutex_unlock(&(sock->mutex));
  return ret;
}

bool asockSetTimeout(asock *sock, u32 timeout)
{
  if(pthread_mutex_lock(&(sock->mutex)) != 0) return false;
  if(sock->busy) { pthread_mutex_unlock(&(sock->mutex)); return false; }
  sock->timeout = timeout;  
  pthread_mutex_unlock(&(sock->mutex));
  return true;
}

//------------------------------------------------------------------------------

bool asockBind(asock *sock, char *ip, u16 port, u32 *errn, char **errs)
{
  if(pthread_mutex_lock(&(sock->mutex)) != 0)
  {
    if(errn) *errn = 0;
    if(errs) *errs = clstrdup("Error locking socket mutex");
    return false;
  }
  if(sock->busy)
  {
    if(errn) *errn = ASOCK_STATUS_BUSY;
    if(errs) *errs = clstrdup("Socket is busy");
    pthread_mutex_unlock(&(sock->mutex));
    return false;
  }

  int x;
  struct addrinfo hints, *results; memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  int status = getaddrinfo(ip, NULL, &hints, &results);
  if(status != 0)
  {
    x = errno;
    if(errn) *errn = (u32)x;
    pthread_mutex_unlock(&(sock->mutex));
    if(errs)
    {
      char buff[1024];
      if(strerror_r(x, buff, 1024) == 0)
        *errs = clstrdup(buff);
      else
       *errs = clstrdup("Unknown error in getaddrinfo()");
    }
    return false;
  }

  status = -1;
  struct addrinfo *curr = results;
  while((status != 0) && (curr != NULL))
  {
    struct sockaddr_in *addr = (struct sockaddr_in *)(results->ai_addr);
    addr->sin_port = htons(port);
    status = bind(sock->fd, (struct sockaddr *)addr, curr->ai_addrlen);
    if(status != 0) { x=errno; curr=curr->ai_next; }
  }
  freeaddrinfo(results);

  if(status == 0)
  {
    if(errn) *errn = 0;
    if(errs) *errs = NULL;
    pthread_mutex_unlock(&(sock->mutex));
    return true;
  }
  else
  {
    if(errn) *errn = (u32)x;
    if(errs)
    {
      char buff[1024];
      if(strerror_r(x, buff, 1024) == 0)
        *errs = clstrdup(buff);
      else
        *errs = clstrdup("Unknown error in bind()");
    }
    pthread_mutex_unlock(&(sock->mutex));
    return false;
  }
}

//------------------------------------------------------------------------------

bool asockListen(asock *sock, u32 *errn, char **errs)
{
  if(pthread_mutex_lock(&(sock->mutex)) != 0)
  {
    if(errn) *errn = 0;
    if(errs) *errs = clstrdup("Error locking socket mutex");
    return false;
  }
  if(sock->busy)
  {
    if(errn) *errn = ASOCK_STATUS_BUSY;
    if(errs) *errs = clstrdup("Socket is busy");
    pthread_mutex_unlock(&(sock->mutex));
    return false;
  }
  if(sock->connected)
  {
    if(errn) *errn = ASOCK_STATUS_CONN;
    if(errs) *errs = clstrdup("Socket already connected");
    pthread_mutex_unlock(&(sock->mutex));
    return false;
  }
  int status = listen(sock->fd, 0);

  if(status == 0)
  {
    if(errn) *errn = 0;
    if(errs) *errs = NULL;
    sock->connected = true;
    pthread_mutex_unlock(&(sock->mutex));
    return true;
  }
  else
  {
    if(errn) *errn = errno;
    if(errs)
    {
      char buff[1024];
      if(strerror_r(errno, buff, 1024) == 0)
        *errs = clstrdup(buff);
      else
        *errs = NULL;
    }
    pthread_mutex_unlock(&(sock->mutex));
    return false;
  }
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

