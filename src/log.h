#ifndef LOG_H
#define LOG_H
#include "core.h"
#include <stdarg.h>

#define LOG_QUIET   0x00
#define LOG_ERROR   BIT(0)
#define LOG_INFO    BIT(1)
#define LOG_CHAT    BIT(2)
#define LOG_WARN    BIT(3)
#define LOG_DEBUG   BIT(4)
#define LOG_COLORS  BIT(5)
#define LOG_REPEAT  BIT(6)
#define LOG_ALL     0x0F
#define LOG_BUFSIZE 8192

typedef struct _LogBuffer {
	cs_char data[LOG_BUFSIZE];
	cs_size offset;
} LogBuffer;

cs_bool Log_Init(void);
void Log_Uninit(void);
void Log_Print(cs_byte flag, cs_str str, va_list *args);

API void Log_Error(cs_str str, ...);
API void Log_Info(cs_str str, ...);
API void Log_Chat(cs_str str, ...);
API void Log_Warn(cs_str str, ...);
API void Log_Debug(cs_str str, ...);

API void Log_SetLevelStr(cs_str str);
VAR cs_byte Log_Flags;
#endif // LOG_H
