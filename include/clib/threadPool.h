// threadPool.h
//==============================================================================
#ifndef CLIB_THREADPOOL_HEADER
#define CLIB_THREADPOOL_HEADER
//==============================================================================
#include <stdio.h>
#include <pthread.h>
#include <clib/types.h>
//==============================================================================

typedef void (*threadPoolFunct)(void *);

typedef struct threadPoolTast_st
{
  threadPoolFunct           funct;
  void                     *arg;
  struct threadPoolTast_st *next;
} threadPoolTask;

typedef struct
{
  u32             size;
  pthread_t      *worker;
  pthread_mutex_t mutex;
  pthread_cond_t  condition;
  i32             load;
  threadPoolTask *origin;
  threadPoolTask *final;
} threadPool;

threadPool *threadPoolCreate(u32 size);
void threadPoolStart(threadPool *pool);
void threadPoolAddWork(threadPool *pool, threadPoolFunct funct, void *arg);
void threadPoolWait(threadPool *pool);
void threadPoolStop(threadPool *pool, bool force, bool wait);
void threadPoolFree(threadPool **pool);

//==============================================================================
#endif //CLIB_THREADPOOL_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

