#include "core.h"
#include "log.h"
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

void Log_Print(int level, const char* str, va_list* args) {
	if(level > Log_Level) return;

	char time[13] = {0};
	char buf[8192] = {0};
	Time_Format(time, 13);

	if(args)
		String_FormatBufVararg(buf, 8192, str, args);
	else
		String_Copy(buf, 8192, str);

	printf("%s [%s] %s\n",
		time,
		Log_Levels[level],
		buf
	);
}

void Log_Error(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(0, str, &args);
	va_end(args);
}

void Log_FormattedError() {
	if(Error_Type == ET_NOERR)
		return;
	Log_Error("%s:%d in %s(...): %s (%d)", Error_GetFile(), Error_Line, Error_GetFunc(), Error_GetString(), Error_Code);
	Error_SetSuccess();
}

void Log_Info(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(1, str, &args);
	va_end(args);
}

void Log_Chat(const char* str, ...) {
	Log_Print(2, str, (va_list*)NULL);
}

void Log_Warn(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(3, str, &args);
	va_end(args);
}

void Log_Debug(const char* str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(4, str, &args);
	va_end(args);
}
