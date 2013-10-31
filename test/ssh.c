#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clib/asock.h>

#define SSHPROXY "serverHostnameHere"
#define PROXYUSR "serverUsernameHere"
#define PROXYPWD "serverPasswordHere"
#define LOCALPRT 4331

bool  verifyHost(char *host, u8 *md5, u8 *sha1);
void  serverTask(asock *sock, void *data, u32 status, char *message);
void  clientTask(asock *sock, void *data, u32 status, char *message);
void *clientTaskRead    = NULL;
u32   clientTaskReadLen = 0;

asock *listener;
int main(int argc, char **argv)
{
  asockSshInitialize();

  asockWorker *worker = asockWorkerCreate(1);
  listener = asockCreate(worker, 5000);
  asock *client = asockCreate(worker, 5000);

  asockWorkerStart(worker);

  u32 errI; char *errS;
  if(asockBind(listener, NULL, LOCALPRT, &errI, &errS) == false)
  {
    asockFree(&listener);
    asockFree(&client);
    asockWorkerStop(worker, false, true);
    asockWorkerFree(&worker);
    printf("Error starting server (bind)(%d - %s)\n", errI, errS);
    return -1;
  }
  if(asockListen(listener, &errI, &errS) == false)
  {
    asockFree(&listener);
    asockFree(&client);
    asockWorkerStop(worker, false, true);
    asockWorkerFree(&worker);
    printf("Error starting server (listen)(%d - %s)\n", errI, errS);
    return -1;
  }
  
  asockAccept(listener, (asockComplete)serverTask, (void *)(intptr_t)0);
  asockSshConnect(client, SSHPROXY, 22, verifyHost, PROXYUSR, PROXYPWD, NULL, NULL, (asockComplete)clientTask, (void *)(intptr_t)0);

  asockWorkerWait(worker);
  asockWorkerStop(worker, false, true);
  asockWorkerFree(&worker);
  asockSshCleanup();

  return 0;
}

char clientBuff[6], serverBuff[6];
void serverTask(asock *sock, void *data, u32 status, char *message)
{
  u32 stage = (u32)((intptr_t)data);
  printf("serverTask stage: %u\n", stage);

  if(status != ASOCK_STATUS_OKAY)
  {
    printf("serverTask error - (%u) \"%s\".\n", status, message);
    if(message) free(message);
    if((stage > 0) && (stage < 3))
    {
      stage = 3;
      asockDisconnect(sock, (asockComplete)serverTask, (void *)(intptr_t)stage);
      return;
    }
    asockFree(&sock);
    return;
  }

  switch(stage++)
  {
    case 0: // client connected, stop listener, send test string
      asockDisconnect(listener, (asockComplete)serverTask, (void *)(intptr_t)3);
      asockWrite(sock, "HELLO", 5, (asockComplete)serverTask, (void *)(intptr_t)stage);
      return;

    case 1: // test string sent, read response
      asockRead(sock, serverBuff, 5, (asockComplete)serverTask, (void *)(intptr_t)stage);
      return;

    case 2: // response read, disconnect
      serverBuff[5] = 0x00;
      printf("Server Read From Client \"%s\"\n", serverBuff);
      asockDisconnect(sock, (asockComplete)serverTask, (void *)(intptr_t)stage);
      return;

    case 3: // disconnected
      asockFree(&sock);
      return;
  }
}

void clientTask(asock *sock, void *data, u32 status, char *message)
{
  u32 stage = (u32)((intptr_t)data);
  printf("clientTask stage: %u\n", stage);

  if(status != ASOCK_STATUS_OKAY)
  {
    printf("clientTask error - (%u) \"%s\".\n", status, message);
    if(message) free(message);
    if((stage > 0) && (stage < 4))
    {
      stage = 4;
      asockSshDisconnect(sock, (asockComplete)clientTask, (void *)(intptr_t)stage);
      return;
    }
    asockFree(&sock);
    return;
  }

  switch(stage++)
  {
    case 0: // Authenticated, Create Tunnel back to our Server
    {
      char *ourExtIp;
      asockGetBound(sock, &ourExtIp, NULL);
      asockSshTunnelTo(sock, ourExtIp, LOCALPRT, (asockComplete)clientTask, (void *)(intptr_t)stage);
      free(ourExtIp);
      return;
    }

    case 1: // Tunnel established, read server test string
      asockRead(sock, clientBuff, 5, (asockComplete)clientTask, (void *)(intptr_t)stage);
      return;

    case 2: // test string read, write response
      clientBuff[5] = 0x00;
      printf("Client Read from Server \"%s\"\n", clientBuff);
      asockWrite(sock, "WORLD", 5, (asockComplete)clientTask, (void *)(intptr_t)stage);
      return;

    case 3: // Completed, Disconnect
      asockSshDisconnect(sock, (asockComplete)clientTask, (void *)(intptr_t)stage);
      return;

    case 4: // Disconnected
      asockFree(&sock);
      return;
  }
}

bool verifyHost(char *host, u8 *md5, u8 *sha1)
{
  printf("Host '%s' MD5  Fingerprint: ", host); for(int i=0; i<16; i++) printf("%02x", (int)(md5 [i])); printf("\n");
  printf("Host '%s' SHA1 Fingerprint: ", host); for(int i=0; i<20; i++) printf("%02x", (int)(sha1[i])); printf("\n");
  return true;
}
