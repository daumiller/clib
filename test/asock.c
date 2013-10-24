#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __linux__
# include <malloc.h>
#endif
#include <clib.h>

//==============================================================================
typedef struct
{
  char *buff;
  u32   index;
} sockData;
sockData *sockDataCreate(u32 index)
{
  sockData *ret = (sockData *)malloc(sizeof(sockData));
  ret->buff = NULL; ret->index = index; return ret;
}
void sockDataFree(sockData *data)
{
  if(data->buff) free(data->buff);
  free(data);
}

//==============================================================================
void serverAcceptor(asock *sock, asock *listener, u32 status, char *message);
void serverPath(asock *sock, sockData *data, u32 status, char *message);
void clientPath(asock *sock, sockData *data, u32 status, char *message);
void errorPath (asock *sock, void *data, u32 status, char *message);

//==============================================================================
int main(int arg, char **argv)
{
  asockWorker *asw = asockWorkerCreate(3);
  asockWorkerStart(asw);

  bool status; u32 statusCode; char *statusDesc;
  asock *sockServer  = asockCreate(asw, 30000);
  asock *sockClient  = asockCreate(asw, 30000);

  status = asockBind(sockServer, "127.0.0.1", 4331, &statusCode, &statusDesc);
  if(!status) { printf("Error binding on server. %u - %s.\n", statusCode, statusDesc); exit(-1); }
  status = asockListen(sockServer, &statusCode, &statusDesc);
  if(!status) { printf("Error listening on server. %u - %s.\n", statusCode, statusDesc); exit(-1); }

  asockAccept (sockServer, (asockComplete)serverAcceptor, (void *)sockServer);
  asockConnect(sockClient, "127.0.0.1", 4331, (asockComplete)clientPath, (void *)sockDataCreate(0));

  asock *sockError; //1) resolution failure; 2) timed-out failure
  sockError = asockCreate(asw,  2000); asockConnect(sockError, "fakesub.samus.org", 80, (asockComplete)errorPath, NULL);
  sockError = asockCreate(asw,  2000); asockConnect(sockError, "8.8.8.8"          , 80, (asockComplete)errorPath, NULL);

  asockWorkerWait(asw);
  asockWorkerStop(asw, false, true);
  asockWorkerFree(&asw);
}

//==============================================================================
void serverAcceptor(asock *sock, asock *listener, u32 status, char *message)
{
  if(status != ASOCK_STATUS_OKAY)
  {
    //if(!OK) sock will be our serverListener
    printf("serverAcceptor Error. %u : \"%s\".\n", status, message);
    if(message) free(message);
    if(sock->connected)
      asockDisconnect(sock, (asockComplete)serverAcceptor, NULL);
    else
      asockFree(&sock);
    return;
  }

  if(listener != NULL)
  {
    printf("Server accepted connection.   Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
    asockWrite(sock, "SYN", 3, (asockComplete)serverPath, (void *)sockDataCreate(0));
    asockDisconnect(listener, (asockComplete)serverAcceptor, NULL);
  }
  else
  {
    printf("Server listener disconnected. Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
    asockFree(&sock);
  }
}

//==============================================================================
void serverPath(asock *sock, sockData *data, u32 status, char *message)
{
  if(status != ASOCK_STATUS_OKAY)
  if(!((data->index == 0xFFFF) && (status == ASOCK_STATUS_DISCONN))) //if(0xFFFF){we don't care about 'already disconnected'}
  {
    printf("Server Path error. %u: \"%s\".\n", status, message); //ONE of these is expected in this test
    if(message) free(message);
    if(data->index != 0xFFFF)
    {
      data->index = 0xFFFF;
      asockDisconnect(sock, (asockComplete)serverPath, data);
    }
    else
    {
      sockDataFree(data);
      asockFree(&sock);
    }
    return;
  }
  
  switch(data->index++)
  {
    case 0:
      printf("Server Connected.             Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
      asockWrite(sock, "SYN", 3, (asockComplete)serverPath, data);
      break;
    case 1:
      printf("Server SYN Sent.              Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
      data->buff = (char *)malloc(3);
      asockRead(sock, data->buff, 3, (asockComplete)serverPath, data);
      break;
    case 2:
      if((data->buff[0] != 'A') || (data->buff[1] != 'C') || (data->buff[2] != 'K'))
      {
        printf("Server ACK BAD (%c%c%c).      Thread 0x%016llx\n", data->buff[0], data->buff[1], data->buff[2], (unsigned long long)(void *)pthread_self());
        data->index = 0xFFFF;
        asockDisconnect(sock, (asockComplete)serverPath, data);
        return;
      }
      printf("Server ACK Received.          Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
      asockRead(sock, data->buff, 3, (asockComplete)serverPath, data); //we're expecting to get dropped here
      return;
    case 0xFFFF:
      printf("Server disconnected.          Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
      sockDataFree(data);
      asockFree(&sock);
      return;
  }
}

//==============================================================================
void clientPath(asock *sock, sockData *data, u32 status, char *message)
{
   if(status != ASOCK_STATUS_OKAY)
  {
    printf("Client Path error. %u: \"%s\".\n", status, message);
    if(message) free(message);
    if(data->index != 0xFFFF)
    {
      data->index = 0xFFFF;
      asockDisconnect(sock, (asockComplete)clientPath, data);
    }
    else
    {
      sockDataFree(data);
      asockFree(&sock);
    }
    return;
  }

  switch(data->index++)
  {
    case 0:
      printf("Client Connected.             Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
      data->buff = (char *)malloc(3);
      asockRead(sock, data->buff, 3, (asockComplete)clientPath, data);
      return;
    case 1:
      if((data->buff[0] != 'S') || (data->buff[1] != 'Y') || (data->buff[2] != 'N'))
      {
        printf("Client SYN BAD (%c%c%c).      Thread 0x%016llx\n", data->buff[0], data->buff[1], data->buff[2], (unsigned long long)(void *)pthread_self());
        data->index = 0xFFFF;
        asockDisconnect(sock, (asockComplete)clientPath, data);
        return;
      }
      printf("Client SYN Received.          Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
      asockWrite(sock, "ACK", 3, (asockComplete)clientPath, data);
      return;
    case 2:
      printf("Client ACK Sent.              Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
      data->index = 0xFFFF;
      asockDisconnect(sock, (asockComplete)clientPath, data);
      return;
    case 0xFFFF:
      printf("Client disconnected.          Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
      sockDataFree(data);
      asockFree(&sock);
      return;
  }
}

//==============================================================================
void errorPath(asock *sock, void *data, u32 status, char *message)
{
  if(status == ASOCK_STATUS_BADHOST)
    printf("Error path failed resolution. Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
  else if(status == ASOCK_STATUS_TIMEOUT)
    printf("Error path timed-out.         Thread 0x%016llx\n", (unsigned long long)(void *)pthread_self());
  else
    printf("Unexpected error path. %u: \"%s\".\n", status, message);

  if(message) free(message);
  asockFree(&sock);
}
