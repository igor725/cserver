#include "core.h"
#include "platform.h"
#include "str.h"
#include "log.h"

cs_uint8 Log_Level = LOG_ERROR | LOG_INFO | LOG_CHAT | LOG_WARN | LOG_DEBUG;

static const char* getName(cs_uint8 flag) {
	switch(flag) {
		case LOG_ERROR:
			return "ERROR";
		case LOG_INFO:
			return "INFO ";
		case LOG_CHAT:
			return "CHAT ";
		case LOG_WARN:
			return "WARN ";
		case LOG_DEBUG:
			return "DEBUG";
	}
	return NULL;
}

void Log_SetLevelStr(const char* str) {
	cs_uint8 level = LOG_ERROR;

	do {
		switch (*str) {
			case 'I':
				level |= LOG_INFO;
				break;
			case 'C':
				level |= LOG_CHAT;
				break;
			case 'W':
				level |= LOG_WARN;
				break;
			case 'D':
				level |= LOG_DEBUG;
				break;
			case 'Q':
				Log_Level = 0;
				return;
		}
	} while(*str++ != '\0');

	Log_Level = level;
}

void Log_Print(cs_uint8 flag, const char* str, va_list* args) {
	if((Log_Level & flag) == 0) return;

	char time[13], buf[8192];
	Time_Format(time, 13);

	if(args)
		String_FormatBufVararg(buf, 8192, str, args);
	else
		String_Copy(buf, 8192, str);

	printf("%s [%s] %s\n",
		time,
		getName(flag),
		buf
	);
}

void Log_Error(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(LOG_ERROR, str, &args);
	va_end(args);
}

void Log_Info(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(LOG_INFO, str, &args);
	va_end(args);
}

void Log_Chat(const char* str, ...) {
	Log_Print(LOG_CHAT, str, (va_list*)NULL);
}

void Log_Warn(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(LOG_WARN, str, &args);
	va_end(args);
}

void Log_Debug(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(LOG_DEBUG, str, &args);
	va_end(args);
}
