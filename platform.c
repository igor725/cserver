#include "core.h"
#include <string.h>

#if defined(WINDOWS)
/*
	WINDOWS MEMORY FUNCTIONS
*/

void* Memory_Alloc(size_t num, size_t size) {
	void* ptr;
	if((ptr = calloc(num, size)) == NULL) {
		Error_Set(ET_SYS, GetLastError());
		return NULL;
	}
	return ptr;
}

void Memory_Copy(void* dst, const void* src, size_t count) {
	memcpy(dst, src, count);
}

void Memory_Fill(void* dst, size_t count, int val) {
	memset(dst, val, count);
}

/*
	WINDOWS ITER FUNCTIONS
*/

#define isDir(iter) ((iter->fileHandle.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0)

bool Iter_Init(const char* dir, const char* ext, dirIter* iter) {
	String_FormatBuf(iter->fmt, 256, "%s\\*.%s", dir, ext);
	if((iter->dirHandle = FindFirstFile(iter->fmt, &iter->fileHandle)) == INVALID_HANDLE_VALUE) {
		iter->state = -1;
		Error_Set(ET_SYS, GetLastError());
		return false;
	}
	iter->cfile = iter->fileHandle.cFileName;
	iter->isDir = isDir(iter);
	iter->state = 1;
	return true;
}

bool Iter_Next(dirIter* iter) {
	if(iter->state != 1)
		return false;

	bool haveFile = FindNextFile(iter->dirHandle, &iter->fileHandle);
	if(haveFile) {
		iter->isDir = isDir(iter);
		iter->cfile = iter->fileHandle.cFileName;
	}

	return haveFile;
}

bool Iter_Close(dirIter* iter) {
	if(iter->state == 0)
		return false;
	FindClose(iter->dirHandle);
	return true;
}

/*
	WINDOWS FILE FUNCTIONS
*/

FILE* File_Open(const char* path, const char* mode) {
	FILE* fp;
	if((fp = fopen(path, mode)) == NULL) {
		Error_Set(ET_SYS, GetLastError());
	}
	return fp;
}

size_t File_Read(void* ptr, size_t size, size_t count, FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE);
		return 0;
	}

	size_t ncount = fread(ptr, size, count, fp);
	if(count != ncount) {
		Error_Set(ET_SYS, GetLastError());
		return ncount;
	}
	return count;
}

bool File_Write(const void* ptr, size_t size, size_t count, FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE);
		return false;
	}
	if(count != fwrite(ptr, size, count, fp)) {
		Error_Set(ET_SYS, GetLastError());
		return false;
	}
	return true;
}

bool File_Error(FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE);
		return true;
	}
	return ferror(fp) > 0;
}

bool File_WriteFormat(FILE* fp, const char* fmt, ...) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE);
		return false;
	}
	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	if(File_Error(fp)) {
		Error_Set(ET_SYS, GetLastError());
		return false;
	}
	return true;
}

bool File_Close(FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE);
		return false;
	}
	if(fclose(fp) != 0) {
		Error_Set(ET_SYS, GetLastError());
		return false;
	}
	return true;
}

/*
	WINDOWS SOCKET FUNCTIONS
*/
bool Socket_Init() {
	WSADATA ws;
	if(WSAStartup(MAKEWORD(1, 1), &ws) == SOCKET_ERROR) {
		Error_Set(ET_SYS, WSAGetLastError());
		return false;
	}
	return true;
}

SOCKET Socket_Bind(const char* ip, ushort port) {
	SOCKET fd;

	if(INVALID_SOCKET == (fd = socket(AF_INET, SOCK_STREAM, 0))) {
		Error_Set(ET_SYS, WSAGetLastError());
		return INVALID_SOCKET;
	}

	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(port);
	if(inet_pton(AF_INET, ip, &ssa.sin_addr.s_addr) <= 0) {
		Error_Set(ET_SYS, WSAGetLastError());
		return INVALID_SOCKET;
	}

	if(bind(fd, (const struct sockaddr*)&ssa, sizeof ssa) == -1) {
		Error_Set(ET_SYS, WSAGetLastError());
		return INVALID_SOCKET;
	}

	if(listen(fd, SOMAXCONN) == -1) {
		Error_Set(ET_SYS, WSAGetLastError());
		return INVALID_SOCKET;
	}

	return fd;
}

void Socket_Close(SOCKET sock) {
	closesocket(sock);
}

/*
	WINDOWS THREAD FUNCTIONS
*/
THREAD Thread_Create(TFUNC func, const TARG lpParam) {
	return CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)func,
		lpParam,
		0,
		NULL
	);
}

bool Thread_IsValid(THREAD th) {
	return th != (THREAD)NULL;
}

bool Thread_SetName(const char* name) {
	return false; //????
}

void Thread_Close(THREAD th) {
	if(th)
		CloseHandle(th);
}

/*
	WINDOWS TIME FUNCTIONS
*/
void Time_Format(char* buf, size_t buflen) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	sprintf_s(buf, buflen, "%02d:%02d:%02d.%03d",
		time.wHour,
		time.wMinute,
		time.wSecond,
		time.wMilliseconds
	);
}
#elif defined(POSIX)
/*
	POSIX MEMORY FUNCTIONS
*/

void* Memory_Alloc(size_t num, size_t size) {
	void* ptr;
	if((ptr = calloc(num, size)) == NULL) {
		Error_Set(ET_SYS, errno);
		return NULL;
	}
	return ptr;
}

void Memory_Copy(void* dst, const void* src, size_t count) {
	memcpy(dst, src, count);
}

void Memory_Fill(void* dst, size_t count, int val) {
	memset(dst, val, count);
}

/*
	POSIX ITER FUNCTIONS
*/

bool Iter_Init(const char* dir, const char* ext, dirIter* iter) {
	if(iter->state > 0)
		return false;

	iter->dirHandle = opendir(dir);
	String_Copy(iter->fmt, 256, ext);
	if(!iter->dirHandle) {
		Error_Set(ET_SYS, errno);
		iter->state = -1;
		return false;
	}

	iter->state = 1;
	Iter_Next(iter);
	return true;
}

static bool checkExtension(const char* filename, const char* ext) {
	const char* _ext = strrchr(filename, '.');
	if((_ext == ext) == NULL) {
		return true;
	} else {
		if(!_ext || !ext) return false;
		return String_Compare(_ext, ext);
	}
}

bool Iter_Next(dirIter* iter) {
	if(iter->state != 1)
		return false;

	if((iter->fileHandle = readdir(iter->dirHandle)) == NULL) {
		iter->state = -1;
		return false;
	} else {
		iter->cfile = iter->fileHandle->d_name;
		iter->isDir = iter->fileHandle->d_type == DT_DIR;
	}

	return true;
}

bool Iter_Close(dirIter* iter) {
	if(iter->state == 0)
		return false;
	closedir(iter->dirHandle);
}

/*
	POSIX FILE FUNCTIONS
*/

FILE* File_Open(const char* path, const char* mode) {
	FILE* fp;
	if((fp = fopen(path, mode)) == NULL) {
		Error_Set(ET_SYS, errno);
	}
	return fp;
}

size_t File_Read(void* ptr, size_t size, size_t count, FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF);
		return 0;
	}

	int ncount;
	if(count != (ncount = fread(ptr, size, count, fp))) {
		Error_Set(ET_SYS, errno);
		return ncount;
	}
	return count;
}

bool File_Write(const void* ptr, size_t size, size_t count, FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF);
		return false;
	}
	if(count != fwrite(ptr, size, count, fp)) {
		Error_Set(ET_SYS, errno);
		return false;
	}
	return true;
}

bool File_Error(FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF);
		return true;
	}
	return ferror(fp) > 0;
}

bool File_WriteFormat(FILE* fp, const char* fmt, ...) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF);
		return false;
	}

	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	if(File_Error(fp)) {
		Error_Set(ET_SYS, errno);
		return false;
	}
	return true;
}

bool File_Close(FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF);
		return false;
	}
	if(fclose(fp) != 0) {
		Error_Set(ET_SYS, errno);
		return false;
	}
	return true;
}

/*
	POSIX SOCKET FUNCTIONS
*/
bool Socket_Init() {
	return true;
}

SOCKET Socket_Bind(const char* ip, ushort port) {
	SOCKET fd;

	if((fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		Error_Set(ET_SYS, errno);
		return INVALID_SOCKET;
	}

	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(port);
	ssa.sin_addr.s_addr = inet_addr(ip);

	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
		Error_Set(ET_SYS, errno);
		return INVALID_SOCKET;
	}

	if(bind(fd, (const struct sockaddr*)&ssa, sizeof(ssa)) == -1) {
		Error_Set(ET_SYS, errno);
		return INVALID_SOCKET;
	}

	if(listen(fd, SOMAXCONN) == -1) {
		Error_Set(ET_SYS, errno);
		return INVALID_SOCKET;
	}

	return fd;
}

void Socket_Close(SOCKET fd) {
	close(fd);
}

/*
	POSIX THREAD FUNCTIONS
*/
THREAD Thread_Create(TFUNC func, const TARG arg) {
	pthread_t thread;
	if(pthread_create(&thread, NULL, func, arg) != 0) {
		Error_Set(ET_SYS, errno);
		return -1;
	}
	return (THREAD)thread;
}

bool Thread_IsValid(THREAD th) {
	return th != (THREAD)-1;
}

bool Thread_SetName(const char* thName) {
	return pthread_setname_np(pthread_self(), thName) == 0;
}

void Thread_Close(THREAD th) {}

/*
	POSIX TIME FUNCTIONS
*/

void Time_Format(char* buf, size_t buflen) {
	struct timeval tv;
	struct tm* tm;
	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);

	if(buflen > 12) {
		sprintf(buf, "%02d:%02d:%02d.%03d",
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec,
			(int) (tv.tv_usec / 1000)
		);
	}
}
#endif
