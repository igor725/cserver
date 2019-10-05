#ifndef PLATFORM_H
#define PLATFORM_H
typedef void* TARG;

#if defined(WINDOWS)
typedef void* THREAD;
typedef uint32_t TRET;
typedef TRET(*TFUNC)(TARG);
typedef CRITICAL_SECTION MUTEX;
typedef struct {
  char fmt[256];
  const char* cfile;
  bool  isDir;
  char  state;
  void* dirHandle;
  WIN32_FIND_DATA fileHandle;
} dirIter;
#define DLIB_EXT "dll"
#elif defined(POSIX)
typedef pthread_t* THREAD;
typedef void*(*TFUNC)(TARG);
typedef void* TRET;
typedef pthread_mutex_t MUTEX;

typedef struct {
  char fmt[256];
  const char* cfile;
  bool isDir;
  char state;
  DIR* dirHandle;
  struct dirent* fileHandle;
} dirIter;
#define DLIB_EXT "so"
#endif

/*
	MEMORY FUNCTIONS
*/

API void* Memory_Alloc(size_t num, size_t size);
API void  Memory_Copy(void* dst, const void* src, size_t count);
API void  Memory_Fill(void* dst, size_t count, int val);
API void  Memory_Free(void* ptr);

/*
  ITER FUNCTIONS
*/

API bool Iter_Init(dirIter* iter, const char* path, const char* ext);
API bool Iter_Next(dirIter* iter);
API bool Iter_Close(dirIter* iter);

/*
	FILE/DIRECTORY FUNCTIONS
*/

API bool File_Rename(const char* path, const char* newpath);
API FILE* File_Open(const char* path, const char* mode);
API size_t File_Read(void* ptr, size_t size, size_t count, FILE* fp);
API size_t File_Write(const void* ptr, size_t size, size_t count, FILE* fp);
API bool File_Error(FILE* fp);
API bool File_WriteFormat(FILE* fp, const char* fmt, ...);
API bool File_Close(FILE* fp);
API bool Directory_Exists(const char* dir);
API bool Directory_Create(const char* dir);
API bool Directory_Ensure(const char* dir);
API bool Directory_SetCurrentDir(const char* path);

/*
	DLIB FUNCTIONS
*/

bool DLib_Load(const char* path, void** lib);
bool DLib_Unload(void* lib);
char* DLib_GetError(char* buf, size_t len);
bool DLib_GetSym(void* lib, const char* sname, void** sym);

/*
	SOCKET FUNCTIONS
*/

bool Socket_Init(void);
API SOCKET Socket_Bind(const char* ip, uint16_t port);
API SOCKET Socket_Accept(SOCKET sock, struct sockaddr_in* addr);
API int Socket_Send(SOCKET sock, char* buf, int len);
API void Socket_Shutdown(SOCKET sock, int how);
API void Socket_Close(SOCKET sock);

/*
	THREAD FUNCTIONS
*/

API THREAD Thread_Create(TFUNC func, const TARG lpParam);
API bool Thread_IsValid(THREAD th);
API bool Thread_SetName(const char* thName);
API void Thread_Close(THREAD th);
API void Thread_Join(THREAD th);

/*
	MUTEX FUNCTIONS
*/

API MUTEX* Mutex_Create(void);
API void Mutex_Free(MUTEX* handle);
API void Mutex_Lock(MUTEX* handle);
API void Mutex_Unlock(MUTEX* handle);

/*
	TIME FUNCTIONS
*/

API void Time_Format(char* buf, size_t len);
API uint64_t Time_GetMSec();

/*
	PROCESS FUNCTIONS
*/

API void Process_Exit(uint32_t ecode);
#endif
