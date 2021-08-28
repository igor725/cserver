#include "core.h"
#include "platform.h"
#include "str.h"
#include "error.h"
#include <stdio.h>

// #include "../../cs_memleak.c"

#if defined(WINDOWS)
HANDLE hHeap = NULL;

cs_bool Memory_Init(void) {
	return (hHeap = HeapCreate(0, 0, 0)) != NULL;
}

void Memory_Uninit(void) {
	if(hHeap) HeapDestroy(hHeap);
}

void *Memory_TryAlloc(cs_size num, cs_size size) {
	return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, num * size);
}

cs_size Memory_GetSize(void *ptr) {
	if(!ptr) return 0;
	return HeapSize(hHeap, 0, ptr);
}

void *Memory_Alloc(cs_size num, cs_size size) {
	void *ptr = Memory_TryAlloc(num, size);
	if(!ptr) {
		Error_PrintSys(true);
	}
	return ptr;
}

void *Memory_TryRealloc(void *oldptr, cs_size new) {
	return HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, oldptr, new);
}

void *Memory_Realloc(void *oldptr, cs_size new) {
	void *newptr = Memory_TryRealloc(oldptr, new);
	if(!newptr) {
		Error_PrintSys(true);
	}
	return newptr;
}

void Memory_Free(void *ptr) {
	HeapFree(hHeap, 0, ptr);
}
#elif defined(UNIX)
#include <stdlib.h>
#include <signal.h>
#include <malloc.h>

cs_bool Memory_Init(void) {return true;}
void Memory_Uninit(void) {}

void *Memory_TryAlloc(cs_size num, cs_size size) {
	return calloc(num, size);
}

cs_size Memory_GetSize(void *ptr) {
	return malloc_usable_size(ptr);
}

void *Memory_Alloc(cs_size num, cs_size size) {
	void *ptr = Memory_TryAlloc(num, size);
	if(!ptr) {
		Error_PrintSys(true);
	}
	return ptr;
}

void *Memory_TryRealloc(void *oldptr, cs_size new) {
	void *newptr = Memory_TryAlloc(1, new);
	if(newptr) {
		Memory_Copy(newptr, oldptr, min(Memory_GetSize(oldptr), new));
		Memory_Free(oldptr);
	}
	return newptr;
}

void *Memory_Realloc(void *oldptr, cs_size new) {
	void *newptr = Memory_TryRealloc(oldptr, new);
	if(!newptr) {
		Error_PrintSys(true);
	}
	return newptr;
}

void Memory_Free(void *ptr) {free(ptr);}
#endif

void Memory_Copy(void *dst, const void *src, cs_size count) {
	cs_byte *u8dst = (cs_byte *)dst,
	*u8src = (cs_byte *)src;
	while(count--) *u8dst++ = *u8src++;
}

void Memory_Fill(void *dst, cs_size count, cs_byte val) {
	cs_byte *u8dst = (cs_byte *)dst;
	while(count--) *u8dst++ = val;
}

cs_bool File_Rename(cs_str path, cs_str newpath) {
#if defined(WINDOWS)
	return (cs_bool)MoveFileExA(path, newpath, MOVEFILE_REPLACE_EXISTING);
#elif defined(UNIX)
	return rename(path, newpath) == 0;
#endif
}

cs_file File_Open(cs_str path, cs_str mode) {
	return fopen(path, mode);
}

cs_file File_ProcOpen(cs_str cmd, cs_str mode) {
#if defined(WINDOWS)
	return _popen(cmd, mode);
#else
	return popen(cmd, mode);
#endif
}

cs_size File_Read(void *ptr, cs_size size, cs_size count, cs_file fp) {
	return fread(ptr, size, count, fp);
}

cs_int32 File_ReadLine(cs_file fp, cs_char *line, cs_int32 len) {
	cs_int32 ch, ilen = len;
	while(len > 1) {
		ch = File_GetChar(fp);
		if(ch == EOF)
			return 0;
		else if(ch == '\n')
			break;
		else if(ch != '\r') {
			*line++ = (cs_char)ch;
			len -= 1;
		}
	}
	*line = '\0';
	if(len == 1) return -1;
	return ilen - len;
}

cs_size File_Write(const void *ptr, cs_size size, cs_size count, cs_file fp) {
	return fwrite(ptr, size, count, fp);
}

cs_int32 File_GetChar(cs_file fp) {
	return fgetc(fp);
}

cs_bool File_Error(cs_file fp) {
	return ferror(fp) != 0;
}

cs_bool File_WriteFormat(cs_file fp, cs_str fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

	return !File_Error(fp);
}

cs_bool File_Flush(cs_file fp) {
	return fflush(fp) == 0;
}

cs_int32 File_Seek(cs_file fp, long offset, cs_int32 origin) {
	return fseek(fp, offset, origin);
}

cs_bool File_Close(cs_file fp) {
	return fclose(fp) != 0;
}

cs_bool File_ProcClose(cs_file fp) {
#if defined(WINDOWS)
	return (cs_bool)_pclose(fp);
#else
	return (cs_bool)pclose(fp);
#endif
}

#if defined(WINDOWS)
cs_bool Socket_Init(void) {
	WSADATA ws;
	return WSAStartup(MAKEWORD(2, 2), &ws) != SOCKET_ERROR;
}

void Socket_Uninit(void) {
	WSACleanup();
}
#else
cs_bool Socket_Init(void) {return true;}
void Socket_Uninit(void) {}
#endif

cs_int32 Socket_SetAddr(struct sockaddr_in *ssa, cs_str ip, cs_uint16 port) {
	ssa->sin_family = AF_INET;
	ssa->sin_port = htons(port);
	return inet_pton(AF_INET, ip, &ssa->sin_addr.s_addr);
}

cs_bool Socket_SetAddrGuess(struct sockaddr_in *ssa, cs_str host, cs_uint16 port) {
	cs_int32 ret;
	if((ret = Socket_SetAddr(ssa, host, port)) == 0) {
		struct addrinfo *addr;
		struct addrinfo hints = {0};
		hints.ai_family = AF_INET;
		hints.ai_socktype = 0;
		hints.ai_protocol = 0;

		cs_char strport[6];
		String_FormatBuf(strport, 6, "%d", port);
		if((ret = getaddrinfo(host, strport, &hints, &addr)) == 0) {
			*ssa = *(struct sockaddr_in *)addr->ai_addr;
			freeaddrinfo(addr);
			return true;
		}
	}
	return ret == 1;
}

Socket Socket_New(void) {
	return socket(AF_INET, SOCK_STREAM, 0);
}

cs_bool Socket_Bind(Socket sock, struct sockaddr_in *addr) {
#if defined(UNIX)
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(cs_int32){1}, 4) == -1) {
		return false;
	}
#endif

	if(bind(sock, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1) {
		return false;
	}

	if(listen(sock, SOMAXCONN) == -1) {
		return false;
	}

	return true;
}

cs_bool Socket_Connect(Socket sock, struct sockaddr_in *addr) {
	socklen_t len = sizeof(struct sockaddr_in);
	return connect(sock, (struct sockaddr *)addr, len) == 0;
}

Socket Socket_Accept(Socket sock, struct sockaddr_in *addr) {
	socklen_t len = sizeof(struct sockaddr_in);
	return accept(sock, (struct sockaddr *)addr, &len);
}

#if defined(WINDOWS)
#define SOCK_DFLAGS 0
#elif defined(UNIX)
#include <unistd.h>
#define SOCK_DFLAGS MSG_NOSIGNAL
#endif

cs_int32 Socket_Receive(Socket sock, cs_char *buf, cs_int32 len, cs_int32 flags) {
	return recv(sock, buf, len, SOCK_DFLAGS | flags);
}

cs_int32 Socket_ReceiveLine(Socket sock, cs_char *line, cs_int32 len) {
	cs_int32 start_len = len;
	cs_char sym;

	while(len > 1) {
		if(Socket_Receive(sock, &sym, 1, MSG_WAITALL) == 1) {
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

cs_int32 Socket_Send(Socket sock, const cs_char *buf, cs_int32 len) {
	return send(sock, buf, len, SOCK_DFLAGS);
}

void Socket_Shutdown(Socket sock, cs_int32 how) {
	shutdown(sock, how);
}

void Socket_Close(Socket sock) {
#if defined(WINDOWS)
	closesocket(sock);
#elif defined(UNIX)
	close(sock);
#endif
}

#if defined(WINDOWS)
#define ISDIR(h) (h.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
cs_bool Iter_Init(DirIter *iter, cs_str dir, cs_str ext) {
	String_FormatBuf(iter->fmt, 256, "%s\\*.%s", dir, ext);
	if((iter->dirHandle = FindFirstFileA(iter->fmt, &iter->fileHandle)) == INVALID_HANDLE_VALUE) {
		if(GetLastError() == ERROR_FILE_NOT_FOUND)
			iter->state = ITER_DONE;
		else
			iter->state = ITER_ERROR;
		return false;
	}

	iter->cfile = iter->fileHandle.cFileName;
	iter->isDir = ISDIR(iter->fileHandle);
	iter->state = ITER_READY;
	return true;
}

cs_bool Iter_Next(DirIter *iter) {
	if(iter->state != ITER_READY)
		return false;

	if(FindNextFileA(iter->dirHandle, &iter->fileHandle)) {
		iter->isDir = ISDIR(iter->fileHandle);
		iter->cfile = iter->fileHandle.cFileName;
		return true;
	}

	iter->state = ITER_DONE;
	return false;
}

void Iter_Close(DirIter *iter) {
	if(iter->dirHandle)
		FindClose(iter->dirHandle);
}
#elif defined(UNIX)
static cs_bool checkExtension(cs_str filename, cs_str ext) {
	cs_str _ext = String_LastChar(filename, '.');
	if(!_ext && !ext) return true;
	if(!_ext || !ext) return false;

	return String_Compare(++_ext, ext);
}

cs_bool Iter_Init(DirIter *iter, cs_str dir, cs_str ext) {
	iter->dirHandle = opendir(dir);
	if(!iter->dirHandle) {
		iter->state = ITER_ERROR;
		return false;
	}

	String_Copy(iter->fmt, 256, ext);
	iter->state = ITER_READY;
	return Iter_Next(iter);
}

cs_bool Iter_Next(DirIter *iter) {
	if(iter->state != ITER_READY)
		return false;

	do {
		if((iter->fileHandle = readdir(iter->dirHandle)) == NULL) {
			iter->state = ITER_DONE;
			return false;
		} else {
			iter->cfile = iter->fileHandle->d_name;
			iter->isDir = iter->fileHandle->d_type == DT_DIR;
		}
	} while(!iter->cfile || !checkExtension(iter->cfile, iter->fmt));

	return true;
}

void Iter_Close(DirIter *iter) {
	if(iter->dirHandle)
		closedir(iter->dirHandle);
}
#endif

#if defined(WINDOWS)
cs_bool Directory_Exists(cs_str path) {
	cs_ulong attr = GetFileAttributesA(path);
	if(attr == INVALID_FILE_ATTRIBUTES) return false;
	return attr & FILE_ATTRIBUTE_DIRECTORY;
}

cs_bool Directory_SetCurrentDir(cs_str path) {
	return (cs_bool)SetCurrentDirectoryA(path);
}

cs_bool Directory_Create(cs_str path) {
	return (cs_bool)CreateDirectoryA(path, NULL);
}
#elif defined(UNIX)
cs_bool Directory_Exists(cs_str path) {
	struct stat ss;
	return stat(path, &ss) == 0 && S_ISDIR(ss.st_mode);
}

cs_bool Directory_SetCurrentDir(cs_str path) {
	return chdir(path) == 0;
}

cs_bool Directory_Create(cs_str path) {
	return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
}
#endif

cs_bool Directory_Ensure(cs_str path) {
	if(Directory_Exists(path)) return true;
	return Directory_Create(path);
}

#if defined(WINDOWS)
cs_bool DLib_Load(cs_str path, void **lib) {
	return (*lib = LoadLibraryA(path)) != NULL;
}

cs_bool DLib_Unload(void *lib) {
	return (cs_bool)FreeLibrary(lib);
}

cs_char *DLib_GetError(cs_char *buf, cs_size len) {
	String_FormatError(GetLastError(), buf, len, NULL);
	return buf;
}

cs_bool DLib_GetSym(void *lib, cs_str sname, void *sym) {
	return (*(void **)sym = (void *)GetProcAddress(lib, sname)) != NULL;
}
#elif defined(UNIX)
#include <dlfcn.h>

cs_bool DLib_Load(cs_str path, void **lib) {
	return (*lib = dlopen(path, RTLD_NOW)) != NULL;
}

cs_bool DLib_Unload(void *lib) {
	return dlclose(lib) == 0;
}

cs_char *DLib_GetError(cs_char *buf, cs_size len) {
	String_Copy(buf, len, dlerror());
	return buf;
}

cs_bool DLib_GetSym(void *lib, cs_str sname, void *sym) {
	return (*(void **)sym = dlsym(lib, sname)) != NULL;
}
#endif

#if defined(WINDOWS)
Thread Thread_Create(TFUNC func, TARG param, cs_bool detach) {
	Thread th;

	if((th = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, param, 0, NULL)) == INVALID_HANDLE_VALUE) {
		ERROR_PRINT(ET_SYS, GetLastError(), true);
	}

	if(detach) {
		Thread_Detach(th);
		return NULL;
	}

	return th;
}

cs_bool Thread_IsValid(Thread th) {
	return th != (Thread)NULL;
}

void Thread_Detach(Thread th) {
	if(!CloseHandle(th)) {
		Error_PrintSys(true);
	}
}

void Thread_Join(Thread th) {
	WaitForSingleObject(th, INFINITE);
	Thread_Detach(th);
}

void Thread_Sleep(cs_uint32 ms) {
	Sleep(ms);
}
#elif defined(UNIX)
Thread Thread_Create(TFUNC func, TARG arg, cs_bool detach) {
	Thread th;
	if(pthread_create(&th, NULL, func, arg) != 0) {
		ERROR_PRINT(ET_SYS, errno, true);
		return (Thread)NULL;
	}

	if(detach) {
		Thread_Detach(th);
		return (Thread)NULL;
	}

	return th;
}

cs_bool Thread_IsValid(Thread th) {
	return th != (Thread)NULL;
}

void Thread_Detach(Thread th) {
	pthread_detach(th);
}

void Thread_Join(Thread th) {
	cs_int32 ret = pthread_join(th, NULL);
	if(ret) {
		ERROR_PRINT(ET_SYS, ret, true);
	}
}

void Thread_Sleep(cs_uint32 ms) {
	usleep(ms * 1000);
}
#endif

#if defined(WINDOWS)
Mutex *Mutex_Create(void) {
	Mutex *ptr = Memory_Alloc(1, sizeof(Mutex));
	InitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(Mutex *handle) {
	DeleteCriticalSection(handle);
	Memory_Free(handle);
}

void Mutex_Lock(Mutex *handle) {
	EnterCriticalSection(handle);
}

void Mutex_Unlock(Mutex *handle) {
	LeaveCriticalSection(handle);
}

Waitable *Waitable_Create(void) {
	Waitable *handle = CreateEventA(NULL, true, false, NULL);
	if(handle == INVALID_HANDLE_VALUE) {
		Error_PrintSys(true);
	}
	return handle;
}

void Waitable_Free(Waitable *handle) {
	if(!CloseHandle(handle)) {
		Error_PrintSys(true);
	}
}

void Waitable_Reset(Waitable *handle) {
	ResetEvent(handle);
}

void Waitable_Signal(Waitable *handle) {
	SetEvent(handle);
}

void Waitable_Wait(Waitable *handle) {
	WaitForSingleObject(handle, INFINITE);
}
#elif defined(UNIX)
#include <fcntl.h>
#include <poll.h>

Mutex *Mutex_Create(void) {
	Mutex *ptr = Memory_Alloc(1, sizeof(Mutex));
	cs_int32 ret = pthread_mutex_init(ptr, NULL);
	if(ret) {
		ERROR_PRINT(ET_SYS, ret, true);
	}
	return ptr;
}

void Mutex_Free(Mutex *handle) {
	cs_int32 ret = pthread_mutex_destroy(handle);
	if(ret) {
		ERROR_PRINT(ET_SYS, ret, true);
	}
	Memory_Free(handle);
}

void Mutex_Lock(Mutex *handle) {
	cs_int32 ret = pthread_mutex_lock(handle);
	if(ret) {
		ERROR_PRINT(ET_SYS, ret, true);
	}
}

void Mutex_Unlock(Mutex *handle) {
	cs_int32 ret = pthread_mutex_unlock(handle);
	if(ret) {
		ERROR_PRINT(ET_SYS, ret, true);
	}
}

Waitable *Waitable_Create(void) {
	Waitable *handle = Memory_Alloc(1, sizeof(Waitable));
	pthread_cond_init(&handle->cond, NULL);
	handle->mutex = Mutex_Create();
	handle->signalled = false;
	return handle;
}

void Waitable_Free(Waitable *handle) {
	pthread_cond_destroy(&handle->cond);
	Mutex_Free(handle->mutex);
	Memory_Free(handle);
}

void Waitable_Signal(Waitable *handle) {
	Mutex_Lock(handle->mutex);
	if(!handle->signalled) {
		handle->signalled = true;
		pthread_cond_signal(&handle->cond);
	}
	Mutex_Unlock(handle->mutex);
}

void Waitable_Reset(Waitable *handle) {
	Mutex_Lock(handle->mutex);
	handle->signalled = false;
	Mutex_Unlock(handle->mutex);
}

void Waitable_Wait(Waitable *handle) {
	Mutex_Lock(handle->mutex);
	while(!handle->signalled)
		pthread_cond_wait(&handle->cond, handle->mutex);
	Mutex_Unlock(handle->mutex);
}
#endif

#if defined(WINDOWS)
void Time_Format(cs_char *buf, cs_size buflen) {
	SYSTEMTIME time;
	GetSystemTime(&time);
	sprintf_s(buf, buflen, "%02d:%02d:%02d.%03d",
		time.wHour,
		time.wMinute,
		time.wSecond,
		time.wMilliseconds
	);
}

cs_uint64 Time_GetMSec(void) {
	FILETIME ft; GetSystemTimeAsFileTime(&ft);
	cs_uint64 time = ft.dwLowDateTime | ((cs_uint64)ft.dwHighDateTime << 32);
	return (time / 10000) + 50491123200000ULL;
}
#elif defined(UNIX)
void Time_Format(cs_char *buf, cs_size buflen) {
	struct timeval tv;
	struct tm *tm;
	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);

	if(buflen > 12) {
		sprintf(buf, "%02d:%02d:%02d.%03d",
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec,
			(cs_int32) (tv.tv_usec / 1000)
		);
	}
}

cs_uint64 Time_GetMSec() {
	struct timeval cur; gettimeofday(&cur, NULL);
	return (cs_uint64)cur.tv_sec * 1000 + 62135596800000ULL + (cur.tv_usec / 1000);
}
#endif

cs_bool Console_BindSignalHandler(TSHND handler) {
#if defined(WINDOWS)
	return (cs_bool)SetConsoleCtrlHandler((PHANDLER_ROUTINE)handler, TRUE);
#elif defined(UNIX)
	return (cs_bool)(signal(SIGINT, handler) != SIG_ERR);
#endif
}

void Process_Exit(cs_int32 code) {
#if defined(WINDOWS)
	ExitProcess(code);
#elif defined(UNIX)
	exit(code);
#endif
}
