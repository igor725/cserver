#ifndef PLATFORMTYPES_H
#define PLATFORMTYPES_H
#include "core.h"
#include <stdio.h>

#if defined(CORE_USE_WINDOWS)
#	define _WINSOCK_DEPRECATED_NO_WARNINGS
#	include <ws2tcpip.h>
#	define TSHND_OK TRUE
#	define MSG_NOSIGNAL 0
#	define MSG_DONTWAIT 0

	typedef WIN32_FIND_DATAA ITER_FILE;
	typedef cs_ulong TRET, TSHND_PARAM;
	typedef void Waitable;
	typedef CRITICAL_SECTION Mutex;
	typedef SOCKET Socket;
	typedef HANDLE Thread, ITER_DIR;
	typedef BOOL TSHND_RET;
#elif defined(CORE_USE_UNIX)
#	include <pthread.h>
#	include <sys/stat.h>
#	include <sys/time.h>
#	include <sys/ioctl.h>
#	include <sys/socket.h>
#	include <netinet/tcp.h>
#	include <errno.h>
#	include <netdb.h>
#	include <arpa/inet.h>
#	include <dirent.h>
#	define INVALID_SOCKET (Socket)-1
#	define SD_SEND SHUT_WR
#	define TSHND_OK

	typedef DIR *ITER_DIR;
	typedef struct dirent *ITER_FILE;
	typedef void *TRET;
	typedef pthread_t Thread;
#	ifdef CORE_USE_DARWIN
		typedef struct _DMutex {
			pthread_mutex_t handle;
			pthread_cond_t cond;
			pthread_t owner;
			cs_uint32 rec;
		} Mutex;
#	else
		typedef struct _UMutex {
			pthread_mutex_t handle;
			pthread_mutexattr_t attr;
		} Mutex;
#	endif
	typedef void TSHND_RET;
	typedef cs_int32 TSHND_PARAM;
	typedef struct _Waitable {
		pthread_cond_t cond;
		Mutex *mutex;
		cs_bool signalled;
	} Waitable;
	typedef cs_int32 Socket;
#endif

typedef cs_int32 cs_error;
typedef FILE *cs_file;
typedef void *TARG;
typedef TRET(*TFUNC)(TARG);
typedef TSHND_RET(*TSHND)(TSHND_PARAM);

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
