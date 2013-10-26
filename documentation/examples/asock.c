#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clib/asock.h>

void  sockTask(asock *sock, void *data, u32 status, char *message);
char *sockTaskQuery   = "GET / HTTP/1.0\r\n\r\n";
void *sockTaskRead    = NULL;
u32   sockTaskReadLen = 0;

int main(int argc, char **argv)
{
  asockWorker *worker = asockWorkerCreate(1);
  asock *sock = asockCreate(worker, 12000);

  asockWorkerStart(worker);
  asockConnect(sock, "icanhazip.com", 80, (asockComplete)sockTask, (void *)(intptr_t)0);

  asockWorkerWait(worker);
  asockWorkerStop(worker, false, true);
  asockWorkerFree(&worker);

  return 0;
}

void sockTask(asock *sock, void *data, u32 status, char *message)
{
  u32 stage = (u32)((intptr_t)data);
  printf("sockTask stage: %u\n", stage);

  if(status != ASOCK_STATUS_OKAY)
  {
    printf("sockTask error - (%u) \"%s\".\n", status, message);
    if(message) free(message);
    asockFree(&sock);
    return;
  }

  switch(stage++)
  {
    case 0: // Connected, Write Query
      asockWrite(sock, sockTaskQuery, strlen(sockTaskQuery), (asockComplete)sockTask, (void *)(intptr_t)stage);
      break;

    case 1: // Queried, Read All Headers
      asockReadUntil(sock, &sockTaskRead, 131072, &sockTaskReadLen, "\r\n\r\n",4, (asockComplete)sockTask, (void *)(intptr_t)stage);
      break;

    case 2: // Skipped Headers, Read Our Content
      free(sockTaskRead);
      asockReadUntil(sock, &sockTaskRead, 131072, &sockTaskReadLen, NULL,0, (asockComplete)sockTask, (void *)(intptr_t)stage);
      break;

    case 3: // Read Content, Finish Up
      //luckily, a CRLF follows the ip address here, otherwise we'd need a realloc to append a terminator
      ((char *)sockTaskRead)[sockTaskReadLen-1] = 0x00;
      printf("External IP: \"%s\"\n", (char *)sockTaskRead);
      free(sockTaskRead);
      asockFree(&sock);
      return;
  }
}
