// asock.h
//==============================================================================
#ifndef CLIB_ASOCK_HEADER
#define CLIB_ASOCK_HEADER
//==============================================================================
#include <pthread.h>
#include <clib/types.h>
#include <clib/threadPool.h>
//==============================================================================

typedef enum
{
  ASOCK_STATUS_OK      = 0x0000, // okay
  ASOCK_STATUS_OKAY    = 0x0000, // okay
  ASOCK_STATUS_BUSY    = 0x0001, // asock operation in-progress
  ASOCK_STATUS_TIMEOUT = 0x0002, // timed out
  ASOCK_STATUS_DISCONN = 0x0003, // disconnected (already)
  ASOCK_STATUS_CONN    = 0x0004, // connected (already)
  ASOCK_STATUS_FAILED  = 0xFFFF  // other/general failure
} asockStatus;

typedef void (*asockComplete)(void *sock, void *data, u32 status, char *message);

typedef struct
{
  pthread_t  thPoll;
  threadPool *pool;
  u32             sockCount;
  pthread_mutex_t sockCountMutex;
  pthread_cond_t  sockCountCond;
  int             fdPoll;
  int             fdPollAbort;
} asockWorker;

typedef struct
{
  int             fd;
  u32             timeout;
  asockComplete   complete;
  void           *completeData;
  void           *workOp;
  asockWorker    *worker;
  bool            connected;
  bool            busy;
  pthread_mutex_t mutex;
  int             workOpEvents;
  u32             workOpPartial;
  u32             workOpNumber;
  u64             workOpTimeout;
  void           *workOpExtra;
#ifdef __linux__
  int             fdTimeout;
#endif
} asock;

asock *asockCreate(asockWorker *worker, u32 timeout);
bool asockIsBusy(asock *sock);
bool asockSetTimeout(asock *sock, u32 timeout);
bool asockBind(asock *sock, char *ip, u16 port, u32 *errn, char **errs);
bool asockListen(asock *sock, u32 *errn, char **errs);
void asockAccept(asock *sock, asockComplete complete, void *data);
void asockConnect(asock *sock, char *host, u16 port, asockComplete complete, void *data);
void asockReadSome(asock *sock, void *buff, u32 max, asockComplete complete, void *data);
void asockRead(asock *sock, void *buff, u32 length, asockComplete complete, void *data);
void asockWrite(asock *sock, void *buff, u32 length, asockComplete complete, void *data);
void asockDisconnect(asock *sock, asockComplete complete, void *data);
void asockFree(asock **sock);
void asockRegister(asock *sock);
void asockUnregister(asock *sock);

asockWorker *asockWorkerCreate(u32 workers);
void asockWorkerStart(asockWorker *worker);
void asockWorkerWait(asockWorker *worker);
void asockWorkerStop(asockWorker *worker, bool force, bool wait);
void asockWorkerFree(asockWorker **worker);

//==============================================================================
#endif //CLIB_ASOCK_HEADER

//==============================================================================
//------------------------------------------------------------------------------
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
