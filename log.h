#ifndef LOG_H
#define LOG_H
#include <stdarg.h>

int Log_level;
void Log_Print(int level, const char* str, va_list* args);
void Log_Error(const char* str, ...);
void Log_Info(const char* str, ...);
void Log_Chat(const char* str, ...);
void Log_Warn(const char* str, ...);
void Log_Debug(const char* str, ...);
void Log_SetLevel(int level);
void Log_FormattedError();
#endif
