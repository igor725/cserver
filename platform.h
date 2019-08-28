#ifndef PLATFORM_H
#define PLATFORM_H

typedef void* THREAD;
typedef void* TARG;

#ifdef _WIN32
typedef int(*TFUNC)(TARG);
typedef int THRET;
#else
typedef void*(*TFUNC)(TARG);
typedef void* THRET;
#endif

/*
	SOCKET FUNCTIONS
*/
bool Socket_Init();
SOCKET Socket_Bind(const char* ip, ushort port);
void Socket_Close(SOCKET sock);

/*
	THREAD FUNCTIONS
*/
THREAD Thread_Create(TFUNC func, TARG lpParam);
void Thread_Close(THREAD th);

/*
	STRING FUNCTIONS
*/
bool String_Compare(const char* str1, const char* str2);
bool String_CaselessCompare(const char* str1, const char* str2);
size_t String_Length(const char* str);
bool String_Copy(char* dst, size_t len, const char* src);
char* String_CopyUnsafe(char* dst, const char* src);
uint String_FormatError(uint code, char* buf, uint buflen);

/*
	TIME FUNCTIONS
*/
void Time_Format(char* buf, size_t len);
#endif
