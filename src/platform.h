#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdio.h>

#if defined(WINDOWS)
#include <ws2tcpip.h>
typedef void *ITER_DIR;
typedef WIN32_FIND_DATA ITER_FILE;
typedef void *Thread;
typedef cs_uint32 TRET;
typedef void Waitable;
typedef CRITICAL_SECTION Mutex;
typedef SOCKET Socket;
#elif defined(POSIX)
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>

typedef DIR *ITER_DIR;
typedef struct dirent *ITER_FILE;
typedef void *TRET;
typedef pthread_t *Thread;
typedef pthread_mutex_t Mutex;
typedef struct {
	cs_int32 pipefd[2];
	cs_char buf[2];
} Waitable;
typedef cs_int32 Socket;
#endif

typedef void *TARG;
typedef TRET(*TFUNC)(TARG);

#define THREAD_FUNC(N) \
static TRET N(TARG param)

enum {
	ITER_INITIAL,
	ITER_READY,
	ITER_DONE,
	ITER_ERROR
};

typedef struct _DirIter {
	cs_byte state;
  cs_char fmt[256];
  cs_str cfile;
  cs_bool isDir;
  ITER_DIR dirHandle;
  ITER_FILE fileHandle;
} DirIter;

#define Memory_Zero(p, c) Memory_Fill(p, c, 0)

void Memory_Init(void);
void Memory_Uninit(void);

API void *Memory_Alloc(cs_size num, cs_size size);
API void *Memory_Realloc(void *buf, cs_size old, cs_size new);
API void  Memory_Copy(void *dst, const void *src, cs_size count);
API void  Memory_Fill(void *dst, cs_size count, cs_byte val);
API void  Memory_Free(void *ptr);

API cs_bool Iter_Init(DirIter *iter, cs_str path, cs_str ext);
API cs_bool Iter_Next(DirIter *iter);
API cs_bool Iter_Close(DirIter *iter);

API cs_bool File_Rename(cs_str path, cs_str newpath);
API FILE *File_Open(cs_str path, cs_str mode);
API cs_size File_Read(void *ptr, cs_size size, cs_size count, FILE *fp);
API cs_int32 File_ReadLine(FILE *fp, cs_char *line, cs_int32 len);
API cs_size File_Write(const void *ptr, cs_size size, cs_size count, FILE *fp);
API cs_int32 File_GetChar(FILE *fp);
API cs_bool File_Error(FILE *fp);
API cs_bool File_WriteFormat(FILE *fp, cs_str fmt, ...);
API cs_bool File_Flush(FILE *fp);
API cs_int32 File_Seek(FILE *fp, long offset, cs_int32 origin);
API cs_bool File_Close(FILE *fp);

API cs_bool Directory_Exists(cs_str dir);
API cs_bool Directory_Create(cs_str dir);
API cs_bool Directory_Ensure(cs_str dir);
API cs_bool Directory_SetCurrentDir(cs_str path);

cs_bool DLib_Load(cs_str path, void **lib);
cs_bool DLib_Unload(void *lib);
cs_char *DLib_GetError(cs_char *buf, cs_size len);
cs_bool DLib_GetSym(void *lib, cs_str sname, void *sym);

cs_bool Socket_Init(void);
API Socket Socket_New(void);
API cs_int32 Socket_SetAddr(struct sockaddr_in *ssa, cs_str ip, cs_uint16 port);
API cs_bool Socket_SetAddrGuess(struct sockaddr_in *ssa, cs_str host, cs_uint16 port);
API cs_bool Socket_Bind(Socket sock, struct sockaddr_in *ssa);
API cs_bool Socket_Connect(Socket sock, struct sockaddr_in *ssa);
API Socket Socket_Accept(Socket sock, struct sockaddr_in *addr);
API cs_int32 Socket_Receive(Socket sock, cs_char *buf, cs_int32 len, cs_int32 flags);
API cs_int32 Socket_ReceiveLine(Socket sock, cs_char *line, cs_int32 len);
API cs_int32 Socket_Send(Socket sock, const cs_char *buf, cs_int32 len);
API void Socket_Shutdown(Socket sock, cs_int32 how);
API void Socket_Close(Socket sock);

API Thread Thread_Create(TFUNC func, const TARG param, cs_bool detach);
API cs_bool Thread_IsValid(Thread th);
API void Thread_Detach(Thread th);
API void Thread_Join(Thread th);

API Mutex *Mutex_Create(void);
API void Mutex_Free(Mutex *handle);
API void Mutex_Lock(Mutex *handle);
API void Mutex_Unlock(Mutex *handle);

API Waitable *Waitable_Create(void);
API void Waitable_Free(Waitable *handle);
API void Waitable_Signal(Waitable *handle);
API void Waitable_Wait(Waitable *handle);
API void Waitable_Reset(Waitable *handle);

API void Time_Format(cs_char *buf, cs_size len);
API cs_uint64 Time_GetMSec(void);

API void Process_Exit(cs_int32 ecode);
#endif // PLATFORM_H
