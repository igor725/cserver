#ifndef PLATFORM_H
#define PLATFORM_H

typedef void* TARG;

#ifdef _WIN32
typedef void* THREAD;
typedef uint THRET;
typedef THRET(*TFUNC)(TARG);
#else
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
	SOCKET FUNCTIONS
*/
bool Socket_Init();
SOCKET Socket_Bind(const char* ip, ushort port);
void Socket_Close(SOCKET sock);

/*
	THREAD FUNCTIONS
*/
THREAD Thread_Create(TFUNC func, const TARG lpParam);
void Thread_Close(THREAD th);

/*
	STRING FUNCTIONS
*/
bool String_Compare(const char* str1, const char* str2);
bool String_CaselessCompare(const char* str1, const char* str2);
size_t String_Length(const char* str);
size_t String_Copy(char* dst, size_t len, const char* src);
char* String_CopyUnsafe(char* dst, const char* src);
uint String_FormatError(uint code, char* buf, uint buflen);
void String_FormatBufVararg(char* buf, size_t len, const char* str, va_list* args);

/*
	TIME FUNCTIONS
*/
void Time_Format(char* buf, size_t len);
#endif
