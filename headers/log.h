#ifndef LOG_H
#define LOG_H
enum {
	LOG_ERROR = 1,
	LOG_INFO = 2,
	LOG_CHAT = 4,
	LOG_WARN = 8,
	LOG_DEBUG = 16
};

void Log_Print(cs_uint8 flag, const char* str, va_list* args);

API void Log_Error(const char* str, ...);
API void Log_Info(const char* str, ...);
API void Log_Chat(const char* str, ...);
API void Log_Warn(const char* str, ...);
API void Log_Debug(const char* str, ...);

API void Log_SetLevelStr(const char* str);
VAR cs_uint8 Log_Level;
#endif
