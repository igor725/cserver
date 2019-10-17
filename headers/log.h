#ifndef LOG_H
#define LOG_H
enum LogFlags {
	LOG_ERROR = 2,
	LOG_INFO = 4,
	LOG_CHAT = 8,
	LOG_WARN = 16,
	LOG_DEBUG = 32
};

void Log_Print(uint8_t flag, const char* str, va_list* args);

API void Log_Error(const char* str, ...);
API void Log_Info(const char* str, ...);
API void Log_Chat(const char* str, ...);
API void Log_Warn(const char* str, ...);
API void Log_Debug(const char* str, ...);

API void Log_SetLevelStr(const char* str);
VAR uint8_t Log_Level;
#endif
