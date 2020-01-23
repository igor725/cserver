#ifndef LOG_H
#define LOG_H
enum {
	LOG_ERROR = 1,
	LOG_INFO = 2,
	LOG_CHAT = 4,
	LOG_WARN = 8,
	LOG_DEBUG = 16
};

void Log_Print(cs_uint8 flag, cs_str str, va_list* args);

API void Log_Error(cs_str str, ...);
API void Log_Info(cs_str str, ...);
API void Log_Chat(cs_str str, ...);
API void Log_Warn(cs_str str, ...);
API void Log_Debug(cs_str str, ...);

API void Log_SetLevelStr(cs_str str);
VAR cs_uint8 Log_Level;
#endif // LOG_H
