#include "core.h"
#include "platform.h"
#include "cserror.h"
#include "str.h"
#include <io.h>

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

void *Memory_TryRealloc(void *oldptr, cs_size new) {
	return HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, oldptr, new);
}

void Memory_Free(void *ptr) {
	HeapFree(hHeap, 0, ptr);
}

cs_error File_Access(cs_str path, cs_int32 mode) {
	return _access_s(path, mode);
}

cs_bool File_Rename(cs_str path, cs_str newpath) {
	return (cs_bool)MoveFileExA(path, newpath, MOVEFILE_REPLACE_EXISTING);
}

cs_file File_ProcOpen(cs_str cmd, cs_str mode) {
	return _popen(cmd, mode);
}

cs_bool File_ProcClose(cs_file fp) {
	return (cs_bool)_pclose(fp);
}

cs_bool Socket_Init(void) {
	WSADATA ws = {0};
	return WSAStartup(MAKEWORD(2, 2), &ws) != SOCKET_ERROR;
}

Socket Socket_New(void) {
	return WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
}

cs_int32 Socket_SetAddr(struct sockaddr_in *ssa, cs_str ip, cs_uint16 port) {
	INT size = sizeof(struct sockaddr_in);

	if(WSAStringToAddressA((LPSTR)ip, AF_INET, NULL, (LPSOCKADDR)ssa, (LPINT)&size))
		return 0;

	ssa->sin_port = htons(port);
	return 1;
}

cs_bool Socket_IsFatal(void) {
	switch(Socket_GetError()) {
		case WSAEWOULDBLOCK:
		case EAGAIN:
		case 0:
			return false;

		default:
			return true;
	}
}

cs_ulong Socket_AvailData(Socket n) {
	cs_ulong avail = 0;
	if(ioctlsocket(n, FIONREAD, &avail) == SOCKET_ERROR)
		Error_PrintSys(false);
	return avail;
}

cs_error Socket_GetError(void) {
	return WSAGetLastError();
}

cs_bool Socket_SetNonBlocking(Socket n, cs_bool state) {
	return ioctlsocket(n, FIONBIO, &(cs_ulong){state}) == 0;
}

void Socket_Close(Socket n) {
	if(closesocket(n) == SOCKET_ERROR)
		Error_PrintSys(false);
}

void Socket_Uninit(void) {
	if(WSACleanup() == SOCKET_ERROR)
		_Error_Print(Socket_GetError(), false);
}

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
	if(iter->dirHandle && !FindClose(iter->dirHandle))
		Error_PrintSys(false);
}

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

Thread Thread_Create(TFUNC func, TARG param, cs_bool detach) {
	Thread th;

	if((th = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, param, 0, NULL)) == INVALID_HANDLE_VALUE)
		Error_PrintSys(true);

	if(detach) {
		Thread_Detach(th);
		return NULL;
	}

	return th;
}

cs_bool Thread_IsValid(Thread th) {
	return th != (Thread)NULL;
}

cs_bool Thread_Signal(Thread th, cs_int32 sig) {
	(void)th; (void)sig;
	return false;
}

void Thread_Detach(Thread th) {
	if(!CloseHandle(th))
		Error_PrintSys(false);
}

void Thread_Join(Thread th) {
	if(WaitForSingleObject(th, INFINITE) == WAIT_FAILED)
		Error_PrintSys(true);
	Thread_Detach(th);
}

void Thread_Sleep(cs_uint32 ms) {
	Sleep(ms);
}

Mutex *Mutex_Create(void) {
	Mutex *ptr = Memory_Alloc(1, sizeof(Mutex));
	InitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(Mutex *mtx) {
	DeleteCriticalSection(mtx);
	Memory_Free(mtx);
}

void Mutex_Lock(Mutex *mtx) {
	EnterCriticalSection(mtx);
}

void Mutex_Unlock(Mutex *mtx) {
	LeaveCriticalSection(mtx);
}

Waitable *Waitable_Create(void) {
	Waitable *handle = CreateEventA(NULL, true, false, NULL);
	if(handle == INVALID_HANDLE_VALUE) Error_PrintSys(true);
	return handle;
}

void Waitable_Free(Waitable *wte) {
	if(!CloseHandle(wte))
		Error_PrintSys(false);
}

void Waitable_Reset(Waitable *wte) {
	if(!ResetEvent(wte))
		Error_PrintSys(true);
}

void Waitable_Signal(Waitable *wte) {
	if(!SetEvent(wte))
		Error_PrintSys(true);
}

void Waitable_Wait(Waitable *wte) {
	if(WaitForSingleObject(wte, INFINITE) == WAIT_FAILED)
		Error_PrintSys(true);
}

cs_bool Waitable_TryWait(Waitable *wte, cs_ulong timeout) {
	return WaitForSingleObject(wte, timeout) == WAIT_OBJECT_0;
}

cs_int32 Time_Format(cs_char *buf, cs_size buflen) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	return sprintf_s(buf, buflen, "%02d:%02d:%02d.%03d",
		time.wHour,
		time.wMinute,
		time.wSecond,
		time.wMilliseconds
	);
}

cs_uint64 Time_GetMSec(void) {
	FILETIME ft; GetSystemTimeAsFileTime(&ft);
	cs_uint64 time = ft.dwLowDateTime | ((cs_uint64)ft.dwHighDateTime << 32);
	return (time - 116444736000000000LL) / 10000;
}

cs_double Time_GetMSecD(void) {
	FILETIME ft; GetSystemTimeAsFileTime(&ft);
	cs_double time = ft.dwLowDateTime / 1.0e7 +
		ft.dwHighDateTime * (4294967296.0 / 1.0e7);
	return (time - 11644473600.0);
}

cs_bool Console_BindSignalHandler(TSHND handler) {
	return (cs_bool)SetConsoleCtrlHandler((PHANDLER_ROUTINE)handler, TRUE);
}

void Process_Exit(cs_int32 code) {
	ExitProcess(code);
}
