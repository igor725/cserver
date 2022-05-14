#ifndef ERROR_H
#define ERROR_H
#include "core.h"

#define _Error_Print(ecode, abort) Error_Print(abort, ecode, __FILE__, __LINE__, __func__)
#define Error_PrintSys(abort) _Error_Print(Error_GetSysCode(), abort)

cs_bool Error_Init(void);
void Error_Uninit(void);
cs_int32 Error_GetSysCode(void);

API void Error_Print(cs_bool abort, cs_int32 code, cs_str file, cs_uint32 line, cs_str func, ...);
#endif // ERROR_H
