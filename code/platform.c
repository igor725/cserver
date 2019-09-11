#include "core.h"
#include <stdio.h>
#include <string.h>

/*
	MEMORY FUNCTIONS
*/

void* Memory_Alloc(size_t num, size_t size) {
	void* ptr;
	if((ptr = calloc(num, size)) == NULL) {
		Error_Set(ET_SYS, GetLastError(), true);
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

void Memory_Free(void* ptr) {
	free(ptr);
}

/*
	Utils
*/

bool Directory_Ensure(const char* path) {
	if(Directory_Exists(path)) return true;
	return Directory_Create(path);
}

#if defined(WINDOWS)
/*
	WINDOWS ITER FUNCTIONS
*/

#define isDir(iter) ((iter->fileHandle.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0)

bool Iter_Init(dirIter* iter, const char* dir, const char* ext) {
	if(iter->state != 0) {
		Error_Set(ET_SERVER, EC_ITERINITED, false);
		return false;
	}

	String_FormatBuf(iter->fmt, 256, "%s\\*.%s", dir, ext);
	if((iter->dirHandle = FindFirstFile(iter->fmt, &iter->fileHandle)) == INVALID_HANDLE_VALUE) {
		uint err = GetLastError();
		if(err != ERROR_FILE_NOT_FOUND) {
			Error_Set(ET_SYS, err, false);
			iter->state = -1;
			return false;
		}
		iter->state = 2;
		return true;
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
	} else
		iter->state = 2;

	return haveFile;
}

bool Iter_Close(dirIter* iter) {
	if(iter->state == 0)
		return false;
	FindClose(iter->dirHandle);
	return true;
}

/*
	WINDOWS FILE/DIRECTORY FUNCTIONS
*/

bool File_Rename(const char* path, const char* newpath) {
	bool succ = MoveFileExA(path, newpath, MOVEFILE_REPLACE_EXISTING);
	if(!succ) {
		Error_Set(ET_SYS, GetLastError(), false);
	}
	return succ;
}

FILE* File_Open(const char* path, const char* mode) {
	FILE* fp;
	if((fp = fopen(path, mode)) == NULL) {
		Error_Set(ET_SYS, GetLastError(), false);
		return NULL;
	}
	return fp;
}

size_t File_Read(void* ptr, size_t size, size_t count, FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE, false);
		return 0;
	}

	size_t ncount = fread(ptr, size, count, fp);
	if(count != ncount) {
		return count;
	}
	return count;
}

size_t File_Write(const void* ptr, size_t size, size_t count, FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE, false);
		return 0;
	}
	size_t wr = fwrite(ptr, size, count, fp);
	if(count != wr) {
		return wr;
	}
	return count;
}

bool File_Error(FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE, false);
		return true;
	}
	return ferror(fp) > 0;
}

bool File_WriteFormat(FILE* fp, const char* fmt, ...) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE, false);
		return false;
	}
	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	if(File_Error(fp)) {
		Error_Set(ET_SYS, GetLastError(), false);
		return false;
	}
	return true;
}

bool File_Close(FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, ERROR_INVALID_HANDLE, false);
		return false;
	}
	if(fclose(fp) != 0) {
		Error_Set(ET_SYS, GetLastError(), false);
		return false;
	}
	return true;
}

bool Directory_Exists(const char* path) {
	uint attr = GetFileAttributes(path);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool Directory_Create(const char* path) {
	bool succ;
	if(!(succ = CreateDirectory(path, NULL))) {
		Error_Set(ET_SYS, GetLastError(), false);
	}
	return succ;
}

/*
	WINDOWS DLIB FUNCTIONS
*/

bool DLib_Load(const char* path, void** lib) {
	if(!(*lib = LoadLibrary(path))) {
		Error_Set(ET_SYS, GetLastError(), false);
		return false;
	}
	return true;
}

bool DLib_GetSym(void* lib, const char* sname, void** sym) {
	if(!(*sym = (void*)GetProcAddress(lib, sname))) {
		Error_Set(ET_SYS, GetLastError(), false);
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
		Error_Set(ET_SYS, WSAGetLastError(), false);
		return false;
	}
	return true;
}

SOCKET Socket_Bind(const char* ip, ushort port) {
	SOCKET fd;

	if(INVALID_SOCKET == (fd = socket(AF_INET, SOCK_STREAM, 0))) {
		Error_Set(ET_SYS, WSAGetLastError(), false);
		return INVALID_SOCKET;
	}

	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(port);
	if(inet_pton(AF_INET, ip, &ssa.sin_addr.s_addr) <= 0) {
		Error_Set(ET_SYS, WSAGetLastError(), false);
		return INVALID_SOCKET;
	}

	if(bind(fd, (const struct sockaddr*)&ssa, sizeof ssa) == -1) {
		Error_Set(ET_SYS, WSAGetLastError(), false);
		return INVALID_SOCKET;
	}

	if(listen(fd, SOMAXCONN) == -1) {
		Error_Set(ET_SYS, WSAGetLastError(), false);
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
	THREAD th = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)func,
		lpParam,
		0,
		NULL
	);

	if(!th) {
		Error_Set(ET_SYS, GetLastError(), false);
		return NULL;
	}

	return th;
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

void Thread_Join(THREAD th) {
	WaitForSingleObject(th, INFINITE);
	Thread_Close(th);
}

/*
	WINDOWS MUTEX FUNCTIONS
*/

MUTEX* Mutex_Create() {
	MUTEX* ptr = (MUTEX*)Memory_Alloc(1, sizeof(MUTEX));
	if(ptr)
		InitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(MUTEX* handle) {
	DeleteCriticalSection(handle);
	Memory_Free(handle);
}

void Mutex_Lock(MUTEX* handle) {
	EnterCriticalSection(handle);
}

void Mutex_Unlock(MUTEX* handle) {
	LeaveCriticalSection(handle);
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

/*
	WINDOWS PROCESS FUNCTIONS
*/

void Process_Exit(uint code) {
	ExitProcess(code);
}
#elif defined(POSIX)
/*
	POSIX ITER FUNCTIONS
*/

bool Iter_Init(dirIter* iter, const char* dir, const char* ext) {
	if(iter->state != 0) {
		Error_Set(ET_SERVER, EC_ITERINITED, false);
		return false;
	}

	iter->dirHandle = opendir(dir);
	if(!iter->dirHandle) {
		Error_Set(ET_SYS, errno, false);
		iter->state = -1;
		return false;
	}

	String_Copy(iter->fmt, 256, ext);
	iter->state = 1;
	Iter_Next(iter);
	return true;
}

static bool checkExtension(const char* filename, const char* ext) {
	const char* _ext = strrchr(filename, '.');
	if(_ext == NULL && ext == NULL) {
		return true;
	} else {
		if(!_ext || !ext) return false;
		return String_Compare(++_ext, ext);
	}
}

bool Iter_Next(dirIter* iter) {
	if(iter->state != 1)
		return false;

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
	if(iter->state == 0)
		return false;
	closedir(iter->dirHandle);
	return true;
}

/*
	POSIX FILE/DIRECTORY FUNCTIONS
*/

bool File_Rename(const char* path, const char* newpath) {
	int ret = rename(path, newpath);
	if(ret != 0) {
		Error_Set(ET_SYS, errno, false);
	}
	return ret == 0;
}

FILE* File_Open(const char* path, const char* mode) {
	FILE* fp;
	if(!(fp = fopen(path, mode))) {
		Error_Set(ET_SYS, errno, false);
		return NULL;
	}
	return fp;
}

size_t File_Read(void* ptr, size_t size, size_t count, FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF, false);
		return 0;
	}

	int ncount;
	if(count != (ncount = fread(ptr, size, count, fp))) {
		Error_Set(ET_SYS, errno, false);
		return ncount;
	}
	return count;
}

size_t File_Write(const void* ptr, size_t size, size_t count, FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF, false);
		return 0;
	}
	size_t wr = fwrite(ptr, size, count, fp);
	if(count != wr) {
		Error_Set(ET_SYS, errno, false);
		return wr;
	}
	return true;
}

bool File_Error(FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF, false);
		return true;
	}
	return ferror(fp) > 0;
}

bool File_WriteFormat(FILE* fp, const char* fmt, ...) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF, false);
		return false;
	}

	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	if(File_Error(fp)) {
		Error_Set(ET_SYS, errno, false);
		return false;
	}

	return true;
}

bool File_Close(FILE* fp) {
	if(!fp) {
		Error_Set(ET_SYS, EBADF, false);
		return false;
	}
	if(fclose(fp) != 0) {
		Error_Set(ET_SYS, errno, false);
		return false;
	}
	return true;
}

bool Directory_Exists(const char* path) {
	struct stat ss;
	return stat(path, &ss) == 0 && S_ISDIR(ss.st_mode);
}

bool Directory_Create(const char* path) {
	if(!mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
		Error_Set(ET_SYS, errno, false);
		return false;
	}
	return true;
}

/*
	POSIX DLIB FUNCTIONS
*/

bool DLib_Load(const char* path, void** lib) {
	*lib = dlopen(path, RTLD_NOW);
	return *lib != NULL;
}

bool DLib_GetSym(void* lib, const char* sname, void** sym) {
	*sym = dlsym(lib, sname);
	return *sym != NULL;
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
		Error_Set(ET_SYS, errno, false);
		return INVALID_SOCKET;
	}

	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(port);
	ssa.sin_addr.s_addr = inet_addr(ip);

	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
		Error_Set(ET_SYS, errno, false);
		return INVALID_SOCKET;
	}

	if(bind(fd, (const struct sockaddr*)&ssa, sizeof(ssa)) == -1) {
		Error_Set(ET_SYS, errno, false);
		return INVALID_SOCKET;
	}

	if(listen(fd, SOMAXCONN) == -1) {
		Error_Set(ET_SYS, errno, false);
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
	THREAD thread = Memory_Alloc(1, sizeof(THREAD));
	if(pthread_create(thread, NULL, func, arg) != 0) {
		Error_Set(ET_SYS, errno, false);
		return NULL;
	}
	return thread;
}

bool Thread_IsValid(THREAD th) {
	return th != NULL;
}

bool Thread_SetName(const char* thName) {
	return pthread_setname_np(pthread_self(), thName) == 0;
}

void Thread_Close(THREAD th) {
	pthread_detach(*th);
	Memory_Free(th);
}

void Thread_Join(THREAD th) {
	int ret = pthread_join(*th, NULL);
	if(ret) {
		Error_Set(ET_SYS, ret, true);
	}
}

/*
	WINDOWS MUTEX FUNCTIONS
*/

MUTEX* Mutex_Create() {
	MUTEX* ptr = (MUTEX*)Memory_Alloc(1, sizeof(MUTEX));
	int ret = pthread_mutex_init(ptr, NULL);
	if(ret) {
		Error_Set(ET_SYS, ret, false);
		return NULL;
	}
	return ptr;
}

void Mutex_Free(MUTEX* handle) {
	int ret = pthread_mutex_destroy(handle);
	if(ret) {
		Error_Set(ET_SYS, ret, false);
	}
	Memory_Free(handle);
}

void Mutex_Lock(MUTEX* handle) {
	int ret = pthread_mutex_lock(handle);
	if(ret) {
		Error_Set(ET_SYS, ret, true);
	}
}

void Mutex_Unlock(MUTEX* handle) {
	int ret = pthread_mutex_unlock(handle);
	if(ret) {
		Error_Set(ET_SYS, ret, true);
	}
}

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

/*
	POSIX PROCESS FUNCTIONS
*/

void Process_Exit(uint code) {
	exit(code);
}
#endif
