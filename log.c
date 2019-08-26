#include "windows.h"
#include "core.h"
#include "stdio.h"

int Log_Level = 3;
const char* const Log_Levels[5] = {
	"ERROR",
	"INFO ",
	"CHAT ",
	"WARN ",
	"DEBUG"
};


int WinErrorStr(int errcode, char* buf, int buflen) {
	int len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errcode, 0, buf, 512, NULL);
	if(len > 0) {
		buf[len - 1] = 0;
		buf[len - 2] = 0;
		return len - 1;
	}
	return len;
}

void Log_SetLevel(int level) {
	Log_Level = min(max(-1, level), 4);
}

void Log_WSAErr(const char* func) {
	int err = WSAGetLastError();
	char buf[512];
	WinErrorStr(err, buf, 512);
	Log_Error("%s: %s (%d)", func, buf, err);
}

void Log_WinErr(const char* func) {
	int err = GetLastError();
	char buf[512];
	WinErrorStr(err, buf, 512);
	Log_Error("%s: %s (%d)", func, buf, err);
}

void Log_Print(int level, const char* str, va_list args) {
	if(level > Log_Level) return;

	char buf[8192];
	SYSTEMTIME time;
	GetSystemTime(&time);
	memset(buf, 0, 8192);
	vsprintf(buf, str, args);
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

void Log_Info(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(1, str, args);
	va_end(args);
}

void Log_Chat(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(2, str, args);
	va_end(args);
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
