#ifndef LOG_H
#define LOG_H
enum LogLevels {
	LOG_ERROR,
	LOG_INFO,
	LOG_CHAT,
	LOG_WARN,
	LOG_DEBUG
};

API void Log_Print(int level, const char* str, va_list* args);
API void Log_Error(const char* str, ...);
API void Log_Info(const char* str, ...);
API void Log_Chat(const char* str, ...);
API void Log_Warn(const char* str, ...);
API void Log_Debug(const char* str, ...);
API void Log_SetLevel(int level);
API int  Log_GetLevel();
#endif
