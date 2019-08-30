#ifndef PLATFORM_H
#define PLATFORM_H
typedef void* TARG;

#if defined(WINDOWS)
typedef void* THREAD;
typedef uint THRET;
typedef THRET(*TFUNC)(TARG);
#elif defined(POSIX)
typedef pthread_t THREAD;
typedef void*(*TFUNC)(TARG);
typedef void* THRET;
#endif

/*
	MEMORY FUNCTIONS
*/
void* Memory_Alloc(size_t num, size_t size);
void  Memory_Copy(void* dst, const void* src, size_t count);
void  Memory_Fill(void* dst, size_t count, int val);

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
