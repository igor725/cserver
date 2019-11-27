#ifndef ERROR_H
#define ERROR_H
#include "platform.h"
enum {
	ET_NOERR = -1,
	ET_SERVER,
	ET_ZLIB,
	ET_SYS,
	ET_STR
};

enum {
	EC_OK,
	EC_MAGIC,
	EC_FILECORR,
	EC_CFGEND,
	EC_CFGLINEPARSE,
	EC_CFGUNK,
	EC_CFGINVGET,
	EC_INVALIDIP
};

#define Error_Print2(etype, ecode, abort) \
Error_Print(etype, ecode, __FILE__, __LINE__, __func__); \
if(abort) { \
	Process_Exit(ecode); \
}
#define Error_PrintF2(etype, ecode, abort, ...) \
Error_PrintF(etype, ecode, __FILE__, __LINE__, __func__, __VA_ARGS__); \
if(abort) { \
	Process_Exit(ecode); \
}
#if defined(WINDOWS)
#  define Error_PrintSys(abort) Error_Print2(ET_SYS, GetLastError(), abort);
#elif defined(POSIX)
#  define Error_PrintSys(abort) Error_Print2(ET_SYS, errno, abort);
#endif

API cs_int32 Error_GetSysCode(void);
API void Error_Print(cs_int32 type, cs_uint32 code, const char* file, cs_uint32 line, const char* func);
API void Error_PrintF(cs_int32 type, cs_uint32 code, const char* file, cs_uint32 line, const char* func, ...);
#endif // ERROR_H
