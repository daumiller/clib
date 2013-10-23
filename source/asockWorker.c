// asockWorker.c
//==============================================================================
#ifdef __linux__
# include <malloc.h>
# include <sys/eventfd.h>
# include <sys/epoll.h>
#elif __APPLE__
# include <stdlib.h>
# include <time.h>
# include <sys/types.h>
# include <sys/event.h>
# include <sys/time.h>
#endif
#include <unistd.h>
#include <string.h>
#include <clib/asock.h>
#include "asockInternal.h"
//==============================================================================
static void *asockWorker_Poll(void *arg);
static void asockWorker_PollAbort(asockWorker *worker);
//==============================================================================

asockWorker *asockWorkerCreate(u32 workers)
{
  asockWorker *worker = (asockWorker *)calloc(sizeof(asockWorker),1);
  worker->pool = threadPoolCreate(workers);
  pthread_mutex_init(&(worker->sockCountMutex), NULL);
  pthread_cond_init (&(worker->sockCountCond ), NULL);
  return worker;
}

void asockWorkerFree(asockWorker **worker)
{
  threadPoolFree(&((*worker)->pool));
  pthread_mutex_destroy(&((*worker)->sockCountMutex));
  pthread_cond_destroy (&((*worker)->sockCountCond ));
  free(*worker);
  *worker = NULL;
}

//------------------------------------------------------------------------------

void asockWorkerStart(asockWorker *worker)
{
  threadPoolStart(worker->pool);
  pthread_create(&(worker->thPoll), NULL, asockWorker_Poll, (void *)worker);
}

void asockWorkerWait(asockWorker *worker)
{
  pthread_mutex_lock(&(worker->sockCountMutex));
  while(worker->sockCount > 0)
    pthread_cond_wait(&(worker->sockCountCond), &(worker->sockCountMutex));
  pthread_mutex_unlock(&(worker->sockCountMutex));

  //there shouldn't really be anything left here, but we'll try to be safe...
  threadPoolWait(worker->pool);
}

void asockWorkerStop(asockWorker *worker, bool force, bool wait)
{
  asockWorker_PollAbort(worker);
  if(force) pthread_cancel(worker->thPoll);
  if(wait)  pthread_join  (worker->thPoll, NULL);
  threadPoolStop(worker->pool, force, wait);
}

//------------------------------------------------------------------------------

static void asockWorker_PollAbort(asockWorker *worker)
{
  if(worker->fdPoll != 0)
  {
#ifdef __linux__
    u64 nonZero = 4331;
    write(worker->fdPollAbort, &nonZero, sizeof(u64));
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, worker->fdPollAbort, EVFILT_TIMER, EV_ADD|EV_ENABLE|EV_ONESHOT, NOTE_NSECONDS, 1, worker);
    struct timespec ts; memset(&ts, 0, sizeof(struct timespec));
    kevent(worker->fdPoll, &ev, 1, NULL, 0, &ts);
#endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void *asockWorker_Poll(void *arg)
{
  asockWorker *worker = (asockWorker *)arg;

#ifdef __linux__
  //create epoll fd
  worker->fdPoll = epoll_create(1);

  //create eventfd fd, for asockWorker abort during epoll_wait
  struct epoll_event event;
  memset(&event, 0, sizeof(struct epoll_event));
  worker->fdPollAbort = eventfd(0, EFD_NONBLOCK);
  event.events   = EPOLLIN | EPOLLET;
  event.data.ptr = worker;
  epoll_ctl(worker->fdPoll, EPOLL_CTL_ADD, worker->fdPollAbort, &event);

  //start polling
  struct epoll_event *events = (struct epoll_event *)calloc(sizeof(struct epoll_event), MAXEVENTS);
  int updates, i; asock *sock; bool aborted = false;
  while(!aborted)
  {
    updates = epoll_wait(worker->fdPoll, events, MAXEVENTS, -1);
    for(i=0; i<updates; i++)
    {
      //abort uses (data.ptr == self)
      if(events[i].data.ptr == worker) { aborted=true; break; }
      sock = (asock *)(events[i].data.ptr);
      //no more updates for this socket until some processing has occured
      epoll_ctl(worker->fdPoll, EPOLL_CTL_DEL, sock->fd       , &event);
      epoll_ctl(worker->fdPoll, EPOLL_CTL_DEL, sock->fdTimeout, &event);
      //add event to work queue (assuming no more than 1 queued operation per socket here; because we store data on sock->)
      sock->workOpEvents = (events[i].data.fd == sock->fdTimeout) ? EPOLLTIMEOUT : events[i].events;
      threadPoolAddWork(worker->pool, (threadPoolFunct)(sock->workOp), sock);
    }
  }

  //cleanup
  free(events);
  close(worker->fdPoll);      worker->fdPoll = 0;
  close(worker->fdPollAbort); worker->fdPollAbort = 0;
#elif __APPLE__
  //create kqueue fd
  worker->fdPoll      = kqueue();
  worker->fdPollAbort = 4331;

  //start polling
  struct kevent evdel[3], *events = (struct kevent *)calloc(sizeof(struct kevent), MAXEVENTS);
  int updates, i; asock *sock; bool aborted = false; struct timespec ts;
  while(!aborted)
  {
    updates = kevent(worker->fdPoll, NULL,0, events,MAXEVENTS, NULL);
    for(i=0; i<updates; i++)
    {
      //abort uses (udata == self)
      if(events[i].udata == worker) { aborted=true; break; }
      sock = (asock *)(events[i].udata);
      //no more updates for this socket until some processing has occured
      EV_SET(evdel+0, sock->fd, EVFILT_TIMER, EV_DELETE, 0, 0, sock);
      EV_SET(evdel+1, sock->fd, EVFILT_READ , EV_DELETE, 0, 0, sock);
      EV_SET(evdel+2, sock->fd, EVFILT_WRITE, EV_DELETE, 0, 0, sock);
      memset(&ts, 0, sizeof(struct timespec));
      kevent(worker->fdPoll, evdel, 3, NULL, 0, &ts);
      //add event to work queue (assuming no more than 1 queued operation per socket here; because we store data on sock->)
      if(events[i].flags & EV_EOF)
      {
        if(events[i].filter == EVFILT_READ ) if(events[i].data == 0) events[i].filter = EVFILT_HUP;
        if(events[i].filter == EVFILT_WRITE)                         events[i].filter = EVFILT_HUP;
      }
      sock->workOpEvents = events[i].filter;
      threadPoolAddWork(worker->pool, (threadPoolFunct)(sock->workOp), sock);
    }
  }

  //cleanup
  free(events);
  close(worker->fdPoll);
  worker->fdPoll      = 0;
  worker->fdPollAbort = 0;
#endif

  return NULL; //pthread_exit(0);
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

