#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <poll.h>
#include <unistd.h>
#include "core.h"
#include "platform.h"
#include "cserror.h"
#include "str.h"

cs_bool Memory_Init(void) {return true;}
void Memory_Uninit(void) {}

void *Memory_TryAlloc(cs_size num, cs_size size) {
	return calloc(num, size);
}

#ifndef CORE_USE_DARWIN
#include <malloc.h>

cs_size Memory_GetSize(void *ptr) {
	return malloc_usable_size(ptr);
}
#else
#include <malloc/malloc.h>

cs_size Memory_GetSize(void *ptr) {
	return malloc_size(ptr);
}
#endif

void *Memory_TryRealloc(void *oldptr, cs_size new) {
	void *newptr = Memory_TryAlloc(1, new);
	if(newptr) {
		Memory_Copy(newptr, oldptr, min(Memory_GetSize(oldptr), new));
		Memory_Free(oldptr);
	}
	return newptr;
}

void Memory_Free(void *ptr) {
	free(ptr);
}

cs_error File_Access(cs_str path, cs_int32 mode) {
	if(access(path, mode) < 0)
		return Thread_GetError();

	return 0;
}

cs_bool File_Rename(cs_str path, cs_str newpath) {
	return rename(path, newpath) == 0;
}

cs_file File_ProcOpen(cs_str cmd, cs_str mode) {
	return popen(cmd, mode);
}

cs_bool File_ProcClose(cs_file fp) {
	return (cs_bool)pclose(fp);
}

cs_bool Socket_Init(void) {
	return true;
}

Socket Socket_New(void) {
	Socket sock;
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		Error_PrintSys(true);
	return sock;
}

cs_int32 Socket_SetAddr(struct sockaddr_in *ssa, cs_str ip, cs_uint16 port) {
	ssa->sin_family = AF_INET;
	ssa->sin_port = htons(port);
	return inet_pton(AF_INET, ip, &ssa->sin_addr.s_addr);
}

cs_bool Socket_IsFatal(void) {
	switch(Socket_GetError()) {
		case 0:
		case EAGAIN:
			return false;

		default:
			return true;
	}
}

cs_error Socket_GetError(void) {
	return Thread_GetError();
}

cs_ulong Socket_AvailData(Socket n) {
	cs_ulong avail = 0;
	if(ioctl(n, FIONREAD, &avail) == -1)
		Error_PrintSys(false);
	return avail;
}

cs_bool Socket_SetNonBlocking(Socket n, cs_bool state) {
	cs_ulong flags = fcntl(n, F_GETFL, 0);
	if(state)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	return fcntl(n, F_SETFL, flags) == 0;
}

void Socket_Close(Socket n) {
	close(n);
}

void Socket_Uninit(void) {}

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
	if(iter->dirHandle && closedir(iter->dirHandle) != 0)
		Error_PrintSys(false);
}

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

Thread Thread_Create(TFUNC func, TARG arg, cs_bool detach) {
	Thread th;
	if(pthread_create(&th, NULL, func, arg) != 0)
		Error_PrintSys(true);

	if(detach) {
		Thread_Detach(th);
		return (Thread)NULL;
	}

	return th;
}

cs_bool Thread_IsValid(Thread th) {
	return th != (Thread)NULL;
}

cs_bool Thread_Signal(Thread th, cs_int32 sig) {
	return pthread_kill(th, sig) == 0;
}

void Thread_Detach(Thread th) {
	cs_int32 ret;
	if((ret = pthread_detach(th)) != 0)
		_Error_Print(ret, true);
}

void Thread_Join(Thread th) {
	cs_int32 ret;
	if((ret = pthread_join(th, NULL)) != 0)
		_Error_Print(ret, true);
}

void Thread_Sleep(cs_uint32 ms) {
	usleep(ms * 1000);
}

Mutex *Mutex_Create(void) {
	Mutex *ptr = Memory_Alloc(1, sizeof(Mutex));
	cs_int32 ret;
	if((ret = pthread_mutexattr_init(&ptr->attr)) != 0)
		_Error_Print(ret, true);
	if((ret = pthread_mutexattr_settype(&ptr->attr, PTHREAD_MUTEX_RECURSIVE)) != 0)
		_Error_Print(ret, true);
	if((ret = pthread_mutex_init(&ptr->handle, &ptr->attr)) != 0)
		_Error_Print(ret, true);
	return ptr;
}

void Mutex_Free(Mutex *mtx) {
	cs_int32 ret;
	if((ret = pthread_mutex_destroy(&mtx->handle)) != 0)
		_Error_Print(ret, true);
	Memory_Free(mtx);
}

void Mutex_Lock(Mutex *mtx) {
	cs_int32 ret;
	if((ret = pthread_mutex_lock(&mtx->handle)) != 0)
		_Error_Print(ret, true);
}

void Mutex_Unlock(Mutex *mtx) {
	cs_int32 ret ;
	if((ret = pthread_mutex_unlock(&mtx->handle)) != 0)
		_Error_Print(ret, true);
}

Waitable *Waitable_Create(void) {
	Waitable *wte = Memory_Alloc(1, sizeof(Waitable));
	cs_int32 ret;
	if((ret = pthread_cond_init(&wte->cond, NULL)) != 0)
		_Error_Print(ret, true);
	return wte;
}

void Waitable_Free(Waitable *wte) {
	cs_int32 ret;
	if((ret = pthread_cond_destroy(&wte->cond)) != 0)
		_Error_Print(ret, true);
	Memory_Free(wte);
}

void Waitable_Signal(Waitable *wte) {
	cs_int32 ret;
	if((ret = pthread_cond_signal(&wte->cond)) != 0)
		_Error_Print(ret, true);
}

void Waitable_Wait(Waitable *wte) {
	cs_int32 ret;
	pthread_mutex_t handle;
	pthread_mutexattr_t attr;

	if((ret = pthread_mutexattr_init(&attr)) != 0)
		_Error_Print(ret, true);
	if((ret = pthread_mutex_init(&handle, &attr)) != 0)
		_Error_Print(ret, true);

	if((ret = pthread_cond_wait(&wte->cond, &handle)) != 0)
		_Error_Print(ret, true);
}

cs_bool Waitable_TryWait(Waitable *wte, cs_ulong timeout) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	cs_ulong secs = timeout / 1000;
	timeout = timeout % 1000;

	cs_ulong add = 0;
	timeout = timeout * 1000 * 1000 + ts.tv_nsec;
	add = timeout / (1000 * 1000 * 1000);
	ts.tv_sec += (add + secs);
	ts.tv_nsec = timeout % (1000 * 1000 * 1000);

	int32_t ret;
	pthread_mutex_t handle;
	pthread_mutexattr_t attr;

	if((ret = pthread_mutexattr_init(&attr)) != 0)
		_Error_Print(ret, true);
	if((ret = pthread_mutex_init(&handle, &attr)) != 0)
		_Error_Print(ret, true);

	return pthread_cond_timedwait(&wte->cond, &handle, &ts) == 0;
}

cs_int32 Time_Format(cs_char *buf, cs_size buflen) {
	struct timeval tv;
	struct tm *tm;
	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	return snprintf(buf, buflen, "%02d:%02d:%02d.%03d",
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec,
		(cs_int32) (tv.tv_usec / 1000)
	);
}

cs_uint64 Time_GetMSec(void) {
	struct timeval cur; gettimeofday(&cur, NULL);
	return (cs_uint64)cur.tv_sec * 1000 + 62135596800000ULL + (cur.tv_usec / 1000);
}

cs_double Time_GetMSecD(void) {
	struct timeval cur; gettimeofday(&cur, NULL);
	return cur.tv_sec + cur.tv_usec / 1.0e6;
}

cs_bool Console_BindSignalHandler(TSHND handler) {
	return (cs_bool)(signal(SIGINT, handler) != SIG_ERR);
}

void Process_Exit(cs_int32 code) {
	exit(code);
}
