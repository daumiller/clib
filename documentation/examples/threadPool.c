#include <stdio.h>
#include <stdlib.h>
#include <clib/threadPool.h>

void workFunct(u32 *i)
{
  printf("Work Unit %u\n", *i);
}

int main(int argc, char **argv)
{
  u32 index[32];

  // create pool, add work
  threadPool *pool = threadPoolCreate(3);
  for(u32 i=0; i<32; i++)
  {
    index[i] = i;
    threadPoolAddWork(pool, (threadPoolFunct)workFunct, (void *)(index+i));
  }

  // start working
  threadPoolStart(pool);

  // wait for work to complete, clean up
  threadPoolWait(pool);
  threadPoolStop(pool, false, true);
  threadPoolFree(&pool);

  return 0;
}
