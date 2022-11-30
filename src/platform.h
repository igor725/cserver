#ifndef PLATFORM_H
#define PLATFORM_H
#include "core.h"
#include "types/platform.h"

#define Memory_Zero(p, c) Memory_Fill(p, c, 0)
#define THREAD_FUNC(N) \
static TRET N(TARG param)

#ifndef CORE_BUILD_PLUGIN
	cs_bool Memory_Init(void);
	void Memory_Uninit(void);

	cs_bool Console_BindSignalHandler(TSHND handler);
#endif

API void *Memory_TryAlloc(cs_size num, cs_size size);
API void *Memory_TryRealloc(void *oldptr, cs_size new);
API cs_size Memory_GetSize(void *ptr);
API void *Memory_Alloc(cs_size num, cs_size size);
API void *Memory_Realloc(void *oldptr, cs_size new);
API void  Memory_Copy(void *dst, const void *src, cs_size count);
API void  Memory_Fill(void *dst, cs_size count, cs_byte val);
API cs_bool Memory_Compare(const cs_byte *src1, const cs_byte *src2, cs_size len);
API void  Memory_Free(void *ptr);

API cs_bool Iter_Init(DirIter *iter, cs_str path, cs_str ext);
API cs_bool Iter_Next(DirIter *iter);
API void Iter_Close(DirIter *iter);

API cs_error File_Access(cs_str path, cs_int32 mode);
API cs_bool File_Rename(cs_str path, cs_str newpath);
API cs_file File_Open(cs_str path, cs_str mode);
API cs_file File_ProcOpen(cs_str cmd, cs_str mode);
API cs_size File_Read(void *ptr, cs_size size, cs_size count, cs_file fp);
API cs_int32 File_ReadLine(cs_file fp, cs_char *line, cs_int32 len);
API cs_size File_Write(const void *ptr, cs_size size, cs_size count, cs_file fp);
API cs_int32 File_GetChar(cs_file fp);
API cs_int32 File_Error(cs_file fp);
API cs_bool File_IsEnd(cs_file fp);
API cs_int32 File_WriteFormat(cs_file fp, cs_str fmt, ...);
API cs_bool File_Flush(cs_file fp);
API cs_int32 File_Seek(cs_file fp, long offset, cs_int32 origin);
API cs_bool File_Close(cs_file fp);
API cs_bool File_ProcClose(cs_file fp);

API cs_bool Directory_Exists(cs_str dir);
API cs_bool Directory_Create(cs_str dir);
API cs_bool Directory_Ensure(cs_str dir);
API cs_bool Directory_SetCurrentDir(cs_str path);

#define DLib_List(names) ((cs_str const[]){names, NULL})
cs_bool DLib_Load(cs_str path, void **lib);
cs_bool DLib_Unload(void *lib);
cs_char *DLib_GetError(cs_char *buf, cs_size len);
cs_bool DLib_GetSym(void *lib, cs_str sname, void *sym);
cs_bool DLib_LoadAll(cs_str const lib[], cs_str const symlist[], void **ctx);

cs_bool Socket_Init(void);
void Socket_Uninit(void);
API Socket Socket_New(void);
API cs_bool Socket_IsFatal(void);
API cs_bool Socket_IsLocal(cs_ulong addr);
API cs_error Socket_GetError(void);
API cs_ulong Socket_AvailData(Socket n);
API cs_bool Socket_SetNonBlocking(Socket n, cs_bool state);
API cs_int32 Socket_SetAddr(struct sockaddr_in *ssa, cs_str ip, cs_uint16 port);
API cs_bool Socket_SetAddrGuess(struct sockaddr_in *ssa, cs_str host, cs_uint16 port);
API cs_bool Socket_Bind(Socket sock, struct sockaddr_in *ssa);
API cs_bool Socket_Connect(Socket sock, struct sockaddr_in *ssa);
API Socket Socket_Accept(Socket sock, struct sockaddr_in *addr);
API cs_int32 Socket_Receive(Socket sock, cs_char *buf, cs_int32 len, cs_int32 flags);
API cs_int32 Socket_Send(Socket sock, const cs_char *buf, cs_int32 len);
API cs_bool Socket_Shutdown(Socket sock, cs_int32 how);
API void Socket_Close(Socket sock);

API Thread Thread_Create(TFUNC func, const TARG param, cs_bool detach);
API cs_bool Thread_IsValid(Thread th);
API cs_bool Thread_Signal(Thread th, cs_int32 sig);
API void Thread_Detach(Thread th);
API void Thread_Join(Thread th);
API void Thread_Sleep(cs_uint32 ms);
API cs_error Thread_GetError(void);

API Mutex *Mutex_Create(void);
API void Mutex_Free(Mutex *mtx);
API void Mutex_Lock(Mutex *mtx);
API void Mutex_Unlock(Mutex *mtx);

API Waitable *Waitable_Create(void);
API void Waitable_Free(Waitable *wte);
API void Waitable_Signal(Waitable *wte);
API void Waitable_Wait(Waitable *wte);
API cs_bool Waitable_TryWait(Waitable *wte, cs_ulong timeout);
API void Waitable_Reset(Waitable *wte);

API cs_int32 Time_Format(cs_char *buf, cs_size len);
API cs_uint64 Time_GetMSec(void);
API cs_double Time_GetMSecD(void);

API void Process_Exit(cs_int32 ecode);
#endif // PLATFORM_H
