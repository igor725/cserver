#ifndef PLATFORM_H
#define PLATFORM_H

typedef void* THREAD;
typedef void* TARG;

#ifdef _WIN32
typedef LPTHREAD_START_ROUTINE TFUNC;
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

/*
	TIME FUNCTIONS
*/
void Time_Format(char* buf, size_t len);
#endif
