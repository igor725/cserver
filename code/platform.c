#include "core.h"
#include "platform.h"
#include "str.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void* Memory_Alloc(size_t num, size_t size) {
	void* ptr;
	if((ptr = calloc(num, size)) == NULL) {
		Error_PrintSys(true);
	}
	return ptr;
}

void* Memory_Realloc(void* buf, size_t old, size_t new) {
	void* pNew = realloc(buf, new);
	if(new > old) {
		size_t diff = new - old;
		void* pStart = ((char*)pNew) + old;
		Memory_Fill(pStart, diff, 0);
	}
	return pNew;
}

void Memory_Copy(void* dst, const void* src, size_t count) {
	memcpy(dst, src, count);
}

void Memory_Fill(void* dst, size_t count, int32_t val) {
	memset(dst, val, count);
}

void Memory_Free(void* ptr) {
	free(ptr);
}

bool File_Rename(const char* path, const char* newpath) {
#if defined(WINDOWS)
	return MoveFileExA(path, newpath, MOVEFILE_REPLACE_EXISTING);
#elif defined(POSIX)
	return rename(path, newpath) == 0;
#endif
}

FILE* File_Open(const char* path, const char* mode) {
	return fopen(path, mode);
}

size_t File_Read(void* ptr, size_t size, size_t count, FILE* fp) {
	return fread(ptr, size, count, fp);
}

int32_t File_ReadLine(FILE* fp, char* line, int32_t len) {
	int32_t bleft = len;

	while(bleft > 1) {
		int32_t ch = File_GetChar(fp);
		if(ch == '\n' || ch == EOF) break;
		if(ch != '\r') {
			*line++ = (char)ch;
			bleft--;
		}
	}

	*line = '\0';
	return len - bleft;
}

size_t File_Write(const void* ptr, size_t size, size_t count, FILE* fp) {
	return fwrite(ptr, size, count, fp);
}

int32_t File_GetChar(FILE* fp) {
	return fgetc(fp);
}

bool File_Error(FILE* fp) {
	return ferror(fp) != 0;
}

bool File_WriteFormat(FILE* fp, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

	return !File_Error(fp);
}

bool File_Flush(FILE* fp) {
	return fflush(fp) == 0;
}

int32_t File_Seek(FILE* fp, long offset, int32_t origin) {
	return fseek(fp, offset, origin);
}

bool File_Close(FILE* fp) {
	return fclose(fp) != 0;
}

bool Socket_Init(void) {
#if defined(WINDOWS)
	WSADATA ws;
	if(WSAStartup(MAKEWORD(1, 1), &ws) == SOCKET_ERROR) {
		return false;
	}
#endif
	return true;
}

int32_t Socket_SetAddr(struct sockaddr_in* ssa, const char* ip, uint16_t port) {
	ssa->sin_family = AF_INET;
	ssa->sin_port = htons(port);
	return inet_pton(AF_INET, ip, &ssa->sin_addr.s_addr);
}

bool Socket_SetAddrGuess(struct sockaddr_in* ssa, const char* host, uint16_t port) {
	int32_t ret;
	if((ret = Socket_SetAddr(ssa, host, port)) == 0) {
		struct addrinfo* addr;
		struct addrinfo hints = {0};
		hints.ai_family = AF_INET;
		hints.ai_socktype = 0;
		hints.ai_protocol = 0;

		char strport[6];
		String_FormatBuf(strport, 6, "%d", port);

		if((ret = getaddrinfo(host, strport, &hints, &addr)) == 0) {
			struct sockaddr_in* new_ssa = (struct sockaddr_in*)addr->ai_addr;
			*ssa = *new_ssa;
			freeaddrinfo(addr);
		}
		return ret == 0;
	}
	return ret == 1;
}

Socket Socket_New() {
	Socket sock;
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		return INVALID_SOCKET;
	}
	return sock;
}

bool Socket_Bind(Socket sock, struct sockaddr_in* addr) {
#if defined(POSIX)
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int32_t){1}, 4) == -1) {
		return false;
	}
#endif

	if(bind(sock, (const struct sockaddr*)addr, sizeof(struct sockaddr_in)) == -1) {
		return false;
	}

	if(listen(sock, SOMAXCONN) == -1) {
		return false;
	}

	return true;
}

bool Socket_Connect(Socket sock, struct sockaddr_in* addr) {
	socklen_t len = sizeof(struct sockaddr_in);
	return connect(sock, (struct sockaddr*)addr, len) == 0;
}

Socket Socket_Accept(Socket sock, struct sockaddr_in* addr) {
	socklen_t len = sizeof(struct sockaddr_in);
	return accept(sock, (struct sockaddr*)addr, &len);
}

int32_t Socket_Receive(Socket sock, char* buf, int32_t len, int32_t flags) {
	return recv(sock, buf, len, flags);
}

int32_t Socket_ReceiveLine(Socket sock, char* line, int32_t len) {
	int32_t start_len = len;
	char sym;

	while(len > 1) {
		if(Socket_Receive(sock, &sym, 1, 0) == 1) {
			if(sym == '\n') {
				*line++ = '\0';
				break;
			} else if(sym != '\r') {
				*line++ = sym;
				--len;
			}
		} else return 0;
	}

	*line = '\0';
	return start_len - len;
}

int32_t Socket_Send(Socket sock, char* buf, int32_t len) {
	return send(sock, buf, len, 0);
}

void Socket_Shutdown(Socket sock, int32_t how) {
	shutdown(sock, how);
}

void Socket_Close(Socket sock) {
#if defined(WINDOWS)
	closesocket(sock);
#elif defined(POSIX)
	close(sock);
#endif
}

#if defined(WINDOWS)
#define isDir(h) (h.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
bool Iter_Init(dirIter* iter, const char* dir, const char* ext) {
	if(iter->state != 0) {
		Error_Print2(ET_SERVER, EC_ITERINITED, false);
		return false;
	}

	String_FormatBuf(iter->fmt, 256, "%s\\*.%s", dir, ext);
	if((iter->dirHandle = FindFirstFile(iter->fmt, &iter->fileHandle)) == INVALID_HANDLE_VALUE) {
		uint32_t err = GetLastError();
		if(err != ERROR_FILE_NOT_FOUND) {
			Error_Print2(ET_SYS, err, false);
			iter->state = -1;
			return false;
		}
		iter->state = 2;
		return true;
	}

	iter->cfile = iter->fileHandle.cFileName;
	iter->isDir = isDir(iter->fileHandle);
	iter->state = 1;
	return true;
}

bool Iter_Next(dirIter* iter) {
	if(iter->state != 1) return false;

	bool haveFile = FindNextFile(iter->dirHandle, &iter->fileHandle);
	if(haveFile) {
		iter->isDir = isDir(iter->fileHandle);
		iter->cfile = iter->fileHandle.cFileName;
	} else
		iter->state = 2;

	return haveFile;
}

bool Iter_Close(dirIter* iter) {
	if(iter->state == 0) return false;
	FindClose(iter->dirHandle);
	return true;
}
#elif defined(POSIX)
static bool checkExtension(const char* filename, const char* ext) {
	const char* _ext = String_LastChar(filename, '.');
	if(!_ext && !ext) return true;
	if(!_ext || !ext) return false;

	return String_Compare(++_ext, ext);
}

bool Iter_Init(dirIter* iter, const char* dir, const char* ext) {
	if(iter->state != 0) {
		Error_Print2(ET_SERVER, EC_ITERINITED, false);
		return false;
	}

	iter->dirHandle = opendir(dir);
	if(!iter->dirHandle) {
		Error_Print2(ET_SYS, errno, false);
		iter->state = -1;
		return false;
	}

	String_Copy(iter->fmt, 256, ext);
	iter->state = 1;
	Iter_Next(iter);
	return true;
}

bool Iter_Next(dirIter* iter) {
	if(iter->state != 1) return false;

	do {
		if((iter->fileHandle = readdir(iter->dirHandle)) == NULL) {
			iter->cfile = NULL;
			iter->isDir = false;
			iter->state = 2;
			return false;
		} else {
			iter->cfile = iter->fileHandle->d_name;
			iter->isDir = iter->fileHandle->d_type == DT_DIR;
		}
	} while(!iter->cfile || !checkExtension(iter->cfile, iter->fmt));

	return true;
}

bool Iter_Close(dirIter* iter) {
	if(iter->state == 0) return false;
	closedir(iter->dirHandle);
	return true;
}
#endif

#if defined(WINDOWS)
bool Directory_Exists(const char* path) {
	uint32_t attr = GetFileAttributes(path);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool Directory_SetCurrentDir(const char* path) {
	return SetCurrentDirectory(path);
}

bool Directory_Create(const char* path) {
	return CreateDirectory(path, NULL);
}
#elif defined(POSIX)
bool Directory_Exists(const char* path) {
	struct stat ss;
	return stat(path, &ss) == 0 && S_ISDIR(ss.st_mode);
}

bool Directory_SetCurrentDir(const char* path) {
	return chdir(path) == 0;
}

bool Directory_Create(const char* path) {
	return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
}
#endif

bool Directory_Ensure(const char* path) {
	if(Directory_Exists(path)) return true;
	return Directory_Create(path);
}

#if defined(WINDOWS)
bool DLib_Load(const char* path, void** lib) {
	return (*lib = LoadLibrary(path)) != NULL;
}

bool DLib_Unload(void* lib) {
	return FreeLibrary(lib);
}

char* DLib_GetError(char* buf, size_t len) {
	String_FormatError(GetLastError(), buf, len, NULL);
	return buf;
}

bool DLib_GetSym(void* lib, const char* sname, void** sym) {
	return (*sym = (void*)GetProcAddress(lib, sname)) != NULL;
}
#elif defined(POSIX)
bool DLib_Load(const char* path, void** lib) {
	return (*lib = dlopen(path, RTLD_NOW)) != NULL;
}

bool DLib_Unload(void* lib) {
	return dlclose(lib) == 0;
}

char* DLib_GetError(char* buf, size_t len) {
	String_Copy(buf, len, dlerror());
	return buf;
}

bool DLib_GetSym(void* lib, const char* sname, void** sym) {
	return (*sym = dlsym(lib, sname)) != NULL;
}
#endif

#if defined(WINDOWS)
Thread Thread_Create(TFUNC func, TARG param) {
	Thread th = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)func,
		param,
		0,
		NULL
	);

	if(!th) {
		Error_Print2(ET_SYS, GetLastError(), true);
		return NULL;
	}

	return th;
}

bool Thread_IsValid(Thread th) {
	return th != (Thread)NULL;
}

void Thread_Close(Thread th) {
	if(th) CloseHandle(th);
}

void Thread_Join(Thread th) {
	WaitForSingleObject(th, INFINITE);
	Thread_Close(th);
}
#elif defined(POSIX)
Thread Thread_Create(TFUNC func, TARG arg) {
	Thread thread = Memory_Alloc(1, sizeof(Thread));
	if(pthread_create(thread, NULL, func, arg) != 0) {
		Error_Print2(ET_SYS, errno, true);
		return NULL;
	}
	return thread;
}

bool Thread_IsValid(Thread th) {
	return th != NULL;
}

void Thread_Close(Thread th) {
	pthread_detach(*th);
	Memory_Free(th);
}

void Thread_Join(Thread th) {
	int32_t ret = pthread_join(*th, NULL);
	if(ret) {
		Error_Print2(ET_SYS, ret, true);
	}
}
#endif

#if defined(WINDOWS)
Mutex* Mutex_Create(void) {
	Mutex* ptr = Memory_Alloc(1, sizeof(Mutex));
	if(!ptr) {
		Error_Print2(ET_SYS, GetLastError(), true);
	}
	InitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(Mutex* handle) {
	DeleteCriticalSection(handle);
	Memory_Free(handle);
}

void Mutex_Lock(Mutex* handle) {
	EnterCriticalSection(handle);
}

void Mutex_Unlock(Mutex* handle) {
	LeaveCriticalSection(handle);
}
#elif defined(POSIX)
Mutex* Mutex_Create(void) {
	Mutex* ptr = Memory_Alloc(1, sizeof(Mutex));
	int32_t ret = pthread_mutex_init(ptr, NULL);
	if(ret) {
		Error_Print2(ET_SYS, ret, true);
		return NULL;
	}
	return ptr;
}

void Mutex_Free(Mutex* handle) {
	int32_t ret = pthread_mutex_destroy(handle);
	if(ret) {
		Error_Print2(ET_SYS, ret, true);
	}
	Memory_Free(handle);
}

void Mutex_Lock(Mutex* handle) {
	int32_t ret = pthread_mutex_lock(handle);
	if(ret) {
		Error_Print2(ET_SYS, ret, true);
	}
}

void Mutex_Unlock(Mutex* handle) {
	int32_t ret = pthread_mutex_unlock(handle);
	if(ret) {
		Error_Print2(ET_SYS, ret, true);
	}
}
#endif

#if defined(WINDOWS)
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

uint64_t Time_GetMSec(void) {
	FILETIME ft; GetSystemTimeAsFileTime(&ft);
	uint64_t time = ft.dwLowDateTime | ((uint64_t)ft.dwHighDateTime << 32);
	return (time / 10000) + 50491123200000ULL;
}
#elif defined(POSIX)
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
			(int32_t) (tv.tv_usec / 1000)
		);
	}
}

uint64_t Time_GetMSec() {
	struct timeval cur; gettimeofday(&cur, NULL);
	return (uint64_t)cur.tv_sec * 1000 + 62135596800000ULL + (cur.tv_usec / 1000);
}
#endif

void Process_Exit(uint32_t code) {
#if defined(WINDOWS)
	ExitProcess(code);
#elif defined(POSIX)
	exit(code);
#endif
}
