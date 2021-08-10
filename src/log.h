#ifndef LOG_H
#define LOG_H
#include "core.h"
#include "platform.h"

enum {
	LOG_QUIET,
	LOG_ERROR = BIT(0),
	LOG_INFO = BIT(1),
	LOG_CHAT = BIT(2),
	LOG_WARN = BIT(3),
	LOG_DEBUG = BIT(4),
	LOG_ALL = 0x0F
};

cs_bool Log_Init(void);
void Log_Uninit(void);
void Log_Print(cs_byte flag, cs_str str, va_list *args);

API void Log_Error(cs_str str, ...);
API void Log_Info(cs_str str, ...);
API void Log_Chat(cs_str str, ...);
API void Log_Warn(cs_str str, ...);
API void Log_Debug(cs_str str, ...);

API void Log_SetLevelStr(cs_str str);
VAR cs_byte Log_Level;
#endif // LOG_H
