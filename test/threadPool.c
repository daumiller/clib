#include <stdio.h>
#ifdef __APPLE__
# include <stdlib.h>
#else
# include <malloc.h>
# define __USE_BSD //for usleep in unistd.h
#endif
#include <unistd.h>
#include <clib.h>

#define PASSFAIL(x) ((x) ? "PASS" : "FAIL")
#define SLOTS   32
#define WORKERS 5

void threadWorkFunct(void *arg);

int main(int argc, char **argv)
{
  threadPool *pool = threadPoolCreate(WORKERS);
  threadPoolStart(pool);
  void *slots[SLOTS];
  for(u32 i=0; i<SLOTS; i++) slots[i] = NULL;
  for(u32 i=0; i<SLOTS; i++) threadPoolAddWork(pool, threadWorkFunct, slots+i);
  void *workers[WORKERS];
  for(u32 i=0; i<WORKERS; i++) workers[i] = (void *)pool->worker[i];
  threadPoolWait(pool);
  threadPoolStop(pool, false, true);
  threadPoolFree(&pool);

  for(u32 i=0; i<SLOTS; i++)
  {
    i32 w = -1;
    for(u32 c=0; c<WORKERS; c++)
      if(slots[i] == workers[c])
        { w=c; break; }
    printf("Slot %2d handled by worker %d.\n", i, w);
  }
  return 0;
}

void threadWorkFunct(void *arg)
{
  void **slot = (void **)arg;
  *slot = (void *)pthread_self();
  usleep(1000);
}

