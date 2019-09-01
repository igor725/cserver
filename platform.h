#ifndef PLATFORM_H
#define PLATFORM_H
typedef void* TARG;

#if defined(WINDOWS)
typedef void* THREAD;
typedef uint THRET;
typedef THRET(*TFUNC)(TARG);

typedef struct {
  char fmt[256];
  const char* cfile;
  bool  isDir;
  char  state;
  void* dirHandle;
  WIN32_FIND_DATA fileHandle;
} dirIter;
#elif defined(POSIX)
typedef pthread_t THREAD;
typedef void*(*TFUNC)(TARG);
typedef void* THRET;
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


/*
  ITER FUNCTIONS
*/

bool Iter_Init(const char* path, const char* ext, dirIter* iter);
bool Iter_Next(dirIter* iter);
bool Iter_Close(dirIter* iter);

/*
	FILE FUNCTIONS
*/

FILE* File_Open(const char* path, const char* mode);
size_t File_Read(void* ptr, size_t size, size_t count, FILE* fp);
bool File_Write(const void* ptr, size_t size, size_t count, FILE* fp);
bool File_Error(FILE* fp);
bool File_WriteFormat(FILE* fp, const char* fmt, ...);
bool File_Close(FILE* fp);

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

/*
	TIME FUNCTIONS
*/
void Time_Format(char* buf, size_t len);
#endif
