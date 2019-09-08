#ifndef PLATFORM_H
#define PLATFORM_H
typedef void* TARG;

#if defined(WINDOWS)
typedef void* THREAD;
typedef uint TRET;
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
#endif

/*
	MEMORY FUNCTIONS
*/

void* Memory_Alloc(size_t num, size_t size);
void  Memory_Copy(void* dst, const void* src, size_t count);
void  Memory_Fill(void* dst, size_t count, int val);
void  Memory_Free(void* ptr);

/*
  ITER FUNCTIONS
*/

bool Iter_Init(dirIter* iter, const char* path, const char* ext);
bool Iter_Next(dirIter* iter);
bool Iter_Close(dirIter* iter);

/*
	FILE/DIRECTORY FUNCTIONS
*/

bool File_Rename(const char* path, const char* newpath);
FILE* File_Open(const char* path, const char* mode);
size_t File_Read(void* ptr, size_t size, size_t count, FILE* fp);
size_t File_Write(const void* ptr, size_t size, size_t count, FILE* fp);
bool File_Error(FILE* fp);
bool File_WriteFormat(FILE* fp, const char* fmt, ...);
bool File_Close(FILE* fp);
bool Directory_Exists(const char* dir);
bool Directory_Create(const char* dir);
bool Directory_Ensure(const char* dir);

/*
	SOCKET FUNCTIONS
*/

bool Socket_Init();
SOCKET Socket_Bind(const char* ip, ushort port);
void Socket_Close(SOCKET sock);

/*
	THREAD FUNCTIONS
*/

THREAD Thread_Create(TFUNC func, const TARG lpParam);
bool Thread_IsValid(THREAD th);
bool Thread_SetName(const char* thName);
void Thread_Close(THREAD th);
void Thread_Join(THREAD th);

/*
	MUTEX FUNCTIONS
*/

MUTEX* Mutex_Create();
void Mutex_Free(MUTEX* handle);
void Mutex_Lock(MUTEX* handle);
void Mutex_Unlock(MUTEX* handle);

/*
	TIME FUNCTIONS
*/

void Time_Format(char* buf, size_t len);

/*
	PROCESS FUNCTIONS
*/
void Process_Exit(uint ecode);
#endif
