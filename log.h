#ifndef LOG_H
#define LOG_H
int Log_level;
void Log_Print(int level, const char* str, va_list args);
void Log_Error(const char* str, ...);
void Log_Info(const char* str, ...);
void Log_Warn(const char* str, ...);
void Log_Debug(const char* str, ...);
void Log_WinErr(const char* func);
void Log_WSAErr(const char* func);
void Log_SetLevel(int level);
#endif
