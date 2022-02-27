#ifndef PLATFORMTYPES_H
#define PLATFORMTYPES_H
#include "core.h"
#include <stdio.h>

#if defined(CORE_USE_WINDOWS)
#include <ws2tcpip.h>
typedef WIN32_FIND_DATAA ITER_FILE;
typedef cs_ulong TRET, TSHND_PARAM;
typedef void Waitable, Semaphore;
typedef CRITICAL_SECTION Mutex;
typedef SOCKET Socket;
typedef HANDLE Thread, ITER_DIR;
typedef BOOL TSHND_RET;
#define TSHND_OK TRUE
#define MSG_NOSIGNAL 0
#define MSG_DONTWAIT 0
#elif defined(CORE_USE_UNIX)
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>

typedef DIR *ITER_DIR;
typedef struct dirent *ITER_FILE;
typedef void *TRET;
typedef pthread_t Thread;
typedef struct _UMutex {
	pthread_mutex_t handle;
	pthread_mutexattr_t attr;
} Mutex;
typedef sem_t Semaphore;
typedef void TSHND_RET;
typedef cs_int32 TSHND_PARAM;
typedef struct {
	pthread_cond_t cond;
	Mutex *mutex;
	cs_bool signalled;
} Waitable;
typedef cs_int32 Socket;
#define INVALID_SOCKET (Socket)-1
#define SD_SEND SHUT_WR
#define TSHND_OK
#endif

typedef cs_int32 cs_error;
typedef FILE *cs_file;
typedef void *TARG;
typedef TRET(*TFUNC)(TARG);
typedef TSHND_RET(*TSHND)(TSHND_PARAM);
#define THREAD_FUNC(N) \
static TRET N(TARG param)

typedef enum _EIterState {
	ITER_INITIAL,
	ITER_READY,
	ITER_DONE,
	ITER_ERROR
} EIterState;

typedef struct _DirIter {
	EIterState state;
	cs_char fmt[256];
	cs_str cfile;
	cs_bool isDir;
	ITER_DIR dirHandle;
	ITER_FILE fileHandle;
} DirIter;
#endif
