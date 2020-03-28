#ifndef LOG_H
#define LOG_H
enum {
	LOG_ERROR = BIT(0),
	LOG_INFO = BIT(1),
	LOG_CHAT = BIT(2),
	LOG_WARN = BIT(3),
	LOG_DEBUG = BIT(4)
};

void Log_Print(cs_uint8 flag, cs_str str, va_list *args);

API void Log_Error(cs_str str, ...);
API void Log_Info(cs_str str, ...);
API void Log_Chat(cs_str str, ...);
API void Log_Warn(cs_str str, ...);
API void Log_Debug(cs_str str, ...);

API void Log_SetLevelStr(cs_str str);
VAR cs_uint8 Log_Level;
#endif // LOG_H
