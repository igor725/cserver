#ifndef ERROR_H
#define ERROR_H
#include "core.h"
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

#define ERROR_PRINT(etype, ecode, abort) \
Error_Print(etype, ecode, __FILE__, __LINE__, __func__); \
if(abort) { \
	Process_Exit(ecode); \
}
#define ERROR_PRINTF(etype, ecode, abort, ...) \
Error_Print(etype, ecode, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
if(abort) { \
	Process_Exit(ecode); \
}
#if defined(WINDOWS)
#  define Error_PrintSys(abort) ERROR_PRINT(ET_SYS, GetLastError(), abort);
#elif defined(UNIX)
#  define Error_PrintSys(abort) ERROR_PRINT(ET_SYS, errno, abort);
#endif

API void Error_Print(cs_int32 type, cs_int32 code, cs_str file, cs_uint32 line, cs_str func, ...);
API cs_int32 Error_GetSysCode(void);
#endif // ERROR_H
