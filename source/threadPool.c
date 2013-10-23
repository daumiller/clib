/// threadPool.c
//==============================================================================
#ifdef __APPLE__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <clib/threadPool.h>
//==============================================================================
static void threadPool_GetWork(threadPool *pool, threadPoolFunct *funct, void **arg);
static void *threadPool_Worker(void *arg);
//==============================================================================

threadPool *threadPoolCreate(u32 size)
{
  threadPool *pool = (threadPool *)malloc(sizeof(threadPool));
  pool->size    = size;
  pool->worker  = (pthread_t *)malloc(sizeof(pthread_t) * size);
  pool->load    = 0;
  pool->running = 0;
  pool->origin  = NULL;
  pool->final   = NULL;
  pthread_mutex_init(&(pool->mutex), NULL);
  pthread_cond_init(&(pool->condition), NULL);
  return pool;
}

void threadPoolFree(threadPool **pool)
{
  pthread_mutex_destroy(&((*pool)->mutex));
  pthread_cond_destroy(&((*pool)->condition));
  free((*pool)->worker);
  threadPoolTask *tmp, *tpt = (*pool)->origin;
  while(tpt != NULL)
  {
    tmp = tpt->next;
    free(tpt);
    tpt = tmp;
  }
  free(*pool);
  *pool = NULL;
}

//------------------------------------------------------------------------------

void threadPoolStart(threadPool *pool)
{
  pool->running = 0;
  if(pool->load < 0) pool->load = 0;
  bool broadcast = (pool->load > 0);
  if(broadcast)
    pthread_mutex_lock(&(pool->mutex));

  for(u32 i=0; i<pool->size; i++)
    pthread_create(&(pool->worker[i]), NULL, threadPool_Worker, pool);

  if(broadcast)
  {
    pthread_cond_broadcast(&(pool->condition));
    pthread_mutex_unlock(&(pool->mutex));
  }
}

void threadPoolWait(threadPool *pool)
{
  pthread_mutex_lock(&(pool->mutex));
  while((pool->load > 0) || (pool->running > 0))
    pthread_cond_wait(&(pool->condition), &(pool->mutex));
  pthread_mutex_unlock(&(pool->mutex));
}

void threadPoolStop(threadPool *pool, bool force, bool wait)
{
  pthread_mutex_lock(&(pool->mutex));
  pool->load = -1;
  pthread_cond_broadcast(&(pool->condition));
  pthread_mutex_unlock(&(pool->mutex));
  if(force) for(u32 i=0; i<pool->size; i++) pthread_cancel(pool->worker[i]);
  if(wait ) for(u32 i=0; i<pool->size; i++) pthread_join  (pool->worker[i], NULL);
}

void threadPoolAddWork(threadPool *pool, threadPoolFunct funct, void *arg)
{
  pthread_mutex_lock(&(pool->mutex));
  if(pool->load == -1) pool->load = 0;
  pool->load++;
  threadPoolTask *tpt = (threadPoolTask *)malloc(sizeof(threadPoolTask));
  tpt->funct = funct;
  tpt->arg   = arg;
  tpt->next  = NULL;
  if(pool->final  != NULL) pool->final->next = tpt;
  if(pool->origin == NULL) pool->origin = tpt;
  pool->final = tpt;
  pthread_cond_broadcast(&(pool->condition));
  pthread_mutex_unlock(&(pool->mutex));
}

//------------------------------------------------------------------------------

static void threadPool_GetWork(threadPool *pool, threadPoolFunct *funct, void **arg)
{
  pool->load--;
  pool->running++;
  *funct = pool->origin->funct;
  *arg   = pool->origin->arg;
  threadPoolTask *tmp = pool->origin;
  pool->origin = pool->origin->next;
  if(pool->final == tmp) pool->final = NULL;
  free(tmp);
}

static void *threadPool_Worker(void *arg)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //killable
  threadPool *pool = (threadPool *)arg;
  threadPoolFunct fn;

  while(1)
  {
    pthread_mutex_lock(&(pool->mutex));
    while(pool->load == 0)
      pthread_cond_wait(&(pool->condition), &(pool->mutex));
    if(pool->load == -1)
    {
      pthread_mutex_unlock(&(pool->mutex));
      return NULL;
    }
    else
    {
      //Get Work; broadcast
      threadPool_GetWork(pool, &fn, &arg);
      pthread_cond_broadcast(&(pool->condition));
      pthread_mutex_unlock(&(pool->mutex));
      //Do Work
      fn(arg);
      //Decrement Running; broadcase
      pthread_mutex_lock(&(pool->mutex));
      pool->running--;
      pthread_cond_broadcast(&(pool->condition));
      pthread_mutex_unlock(&(pool->mutex));
    }
  }
  return NULL;
}

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

