// asock.h
//==============================================================================
#ifndef CLIB_ASOCK_HEADER
#define CLIB_ASOCK_HEADER
//==============================================================================
#include <pthread.h>
#include <clib/types.h>
#include <clib/threadPool.h>
//==============================================================================
#define ASOCK_STATUS_OK      0x0000 // okay
#define ASOCK_STATUS_OKAY    0x0000 // okay
#define ASOCK_STATUS_BUSY    0x0001 // asock operation in-progress
#define ASOCK_STATUS_TIMEOUT 0x0002 // timed out
#define ASOCK_STATUS_DISCONN 0x0003 // disconnected (already)
#define ASOCK_STATUS_CONN    0x0004 // connected (already)
#define ASOCK_STATUS_BADHOST 0x0005 // unable to resolve/route
#define ASOCK_STATUS_HITMAX  0x0006 // read maximum amount of data in an until
#define ASOCK_STATUS_SSH     0x8000 // general SSH error
#define ASOCK_STATUS_SSHHAND 0x8001 // SSH handshake error
#define ASOCK_STATUS_SSHHOST 0x8002 // host rejected by SSH verification function
#define ASOCK_STATUS_SSHUSER 0x8003 // bad SSH user
#define ASOCK_STATUS_SSHAUTH 0x8004 // SSH authentication failed
#define ASOCK_STATUS_SSHCHAN 0x8005 // SSH host closed channel
#define ASOCK_STATUS_FAILED  0xFFFF // other/general failure
//------------------------------------------------------------------------------
#ifdef _CLIB_SSH_
#include <libssh2.h>
typedef bool (*asockSshHostVerify)(char *host, u8 *md5, u8 *sha1);
#endif

typedef void (*asockComplete)(void *sock, void *data, u32 status, char *message);

typedef struct
{
  pthread_t       thPoll;
  threadPool     *pool;
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
  u8             *readBuff;
  u32             readBuffLen;
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
#ifdef _CLIB_SSH_
  LIBSSH2_SESSION  *sshSession;
  LIBSSH2_CHANNEL  *sshChannel;
  LIBSSH2_LISTENER *sshListener;
#endif
} asock;

asock *asockCreate(asockWorker *worker, u32 timeout);
bool asockIsBusy(asock *sock);
bool asockSetTimeout(asock *sock, u32 timeout);
bool asockBind(asock *sock, char *ip, u16 port, u32 *errn, char **errs);
bool asockGetBound(asock *sock, char **ip, u16 *port);
bool asockListen(asock *sock, u32 *errn, char **errs);
void asockAccept(asock *sock, asockComplete complete, void *data);
void asockConnect(asock *sock, char *host, u16 port, asockComplete complete, void *data);
void asockReadSome(asock *sock, void *buff, u32 max, u32 *read, asockComplete complete, void *data);
void asockRead(asock *sock, void *buff, u32 length, asockComplete complete, void *data);
void asockReadUntil(asock *sock, void **recv, u32 max, u32 *read, void *until, u8 untilLen, asockComplete complete, void *data);
void asockWrite(asock *sock, void *buff, u32 length, asockComplete complete, void *data);
void asockDisconnect(asock *sock, asockComplete complete, void *data);
void asockFree(asock **sock);

#ifdef _CLIB_SSH_
//NOTE: asockSsh*() functions can be worker-blocking (they do have the ability to clog a pool)
bool asockSshInitialize();
void asockSshCleanup();
void asockSshConnect(asock *sock, char *host, u16 port, asockSshHostVerify verify, char *user, char *pass, char *keypub, char *keypriv, asockComplete complete, void *data);
void asockSshTunnelTo(asock *sock, char *host3rd, u16 port3rd, asockComplete complete, void *data);
void asockSshTunnelFromListen(asock *sock, char *hostBind, u16 portBind, u16 *portUsed, asockComplete complete, void *data);
void asockSshTunnelFromAccept(asock *sock, asockComplete complete, void *data);
void asockSshTunnelFromUnlisten(asock *sock, asockComplete complete, void *data);
void asockSshDisconnect(asock *sock, asockComplete complete, void *data);
#endif

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
