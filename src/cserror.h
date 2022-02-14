#ifndef ERROR_H
#define ERROR_H
#include "core.h"
#include "platform.h"

#define ERROR_PRINT(ecode, abort) \
Error_Print(ecode, __FILE__, __LINE__, __func__); \
if(abort) { \
	Process_Exit(ecode); \
}

#define ERROR_PRINTF(ecode, abort, ...) \
Error_Print(ecode, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
if(abort) { \
	Process_Exit(ecode); \
}

#if defined(CORE_USE_WINDOWS)
#  define Error_PrintSys(abort) ERROR_PRINT(GetLastError(), abort);
#elif defined(CORE_USE_UNIX)
#  define Error_PrintSys(abort) ERROR_PRINT(errno, abort);
#endif

cs_bool Error_Init(void);
void Error_Uninit(void);

API void Error_Print(cs_int32 code, cs_str file, cs_uint32 line, cs_str func, ...);
API cs_int32 Error_GetSysCode(void);
#endif // ERROR_H
