#include "core.h"
#include "log.h"

uint8_t Log_Level = LOG_ERROR | LOG_INFO | LOG_CHAT | LOG_WARN;

static const char* getName(uint8_t flag) {
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
	uint8_t level = 0;

	do {
		switch (*str) {
			case 'E':
				level |= LOG_ERROR;
				break;
				case 'I':
					level |= LOG_INFO;
					break;
				case 'C':
					level |= LOG_ERROR;
					break;
				case 'W':
					level |= LOG_WARN;
					break;
				case 'D':
					level |= LOG_DEBUG;
					break;

		}
	} while(*str++ != '\0');

	Log_Level = level;
}

void Log_Print(uint8_t flag, const char* str, va_list* args) {
	if((Log_Level & flag) == 0) return;

	char time[13] = {0};
	char buf[8192] = {0};
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
