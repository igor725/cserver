#include "windows.h"
#include "core.h"
#include "stdio.h"
#include "error.h"

int Log_Level = 3;
const char* const Log_Levels[5] = {
	"ERROR",
	"INFO ",
	"CHAT ",
	"WARN ",
	"DEBUG"
};


void Log_SetLevel(int level) {
	Log_Level = min(max(-1, level), 4);
}

void Log_Print(int level, const char* str, va_list args) {
	if(level > Log_Level) return;

	char buf[8192] = {0};
	SYSTEMTIME time;
	GetSystemTime(&time);

	if(args)
		vsprintf(buf, str, args);
	else
		strcpy(buf, str);

	printf("%02d:%02d:%02d.%03d [%s] %s\n",
		time.wHour,
		time.wMinute,
		time.wSecond,
		time.wMilliseconds,
		Log_Levels[level],
		buf
	);
}

void Log_Error(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(0, str, args);
	va_end(args);
}

void Log_FormattedError() {
	Log_Error("%s(...): %s (%d)", Error_GetFunc(), Error_GetString(), Error_GetCode());
	Error_SetSuccess();
}

void Log_Info(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(1, str, args);
	va_end(args);
}

void Log_Chat(const char* str, ...) {
	Log_Print(2, str, NULL);
}

void Log_Warn(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(3, str, args);
	va_end(args);
}

void Log_Debug(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(4, str, args);
	va_end(args);
}
