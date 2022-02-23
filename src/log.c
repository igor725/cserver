#include "core.h"
#include "str.h"
#include "platform.h"
#include "log.h"
#include "event.h"
#include "consoleio.h"

cs_byte Log_Flags = LOG_ALL;
static Mutex *logMutex = NULL;

#define MKCOL(c, t) Log_Flags&LOG_COLORS?"\x1B["c"m"t"\x1B[0m":t

INL static cs_str getName(cs_byte flag) {
	switch(flag) {
		case LOG_ERROR:
			return MKCOL("1;34", "ERROR");
		case LOG_INFO:
			return MKCOL("1;32", "INFO ");
		case LOG_CHAT:
			return MKCOL("1;33", "CHAT ");
		case LOG_WARN:
			return MKCOL("35", "WARN ");
		case LOG_DEBUG:
			return MKCOL("1;34", "DEBUG");
	}
	return NULL;
}

cs_bool Log_Init(void) {
	return (logMutex = Mutex_Create()) != NULL;
}

void Log_Uninit(void) {
	if(logMutex) Mutex_Free(logMutex);
}

void Log_SetLevelStr(cs_str str) {
	cs_byte level = LOG_ERROR;

	do {
		switch (*str) {
			case 'c':
				level |= LOG_COLORS;
				break;
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
				level = LOG_QUIET;
				break;
		}
	} while(*str++ != '\0');

	Log_Flags = level;
}

static LogBuffer buffer = {
	.offset = 0
};

void Log_Print(cs_byte flag, cs_str str, va_list *args) {
	if(Log_Flags & flag) {
		Mutex_Lock(logMutex);

		cs_int32 ret;
		if((ret = Time_Format(buffer.data, LOG_BUFSIZE)) > 0)
			buffer.offset = ret;

		if((ret = String_FormatBuf(buffer.data + buffer.offset,
			LOG_BUFSIZE - buffer.offset, " [%s] ", getName(flag)
		)) > 0)
			buffer.offset += ret;

		if(args) {
			if((ret = String_FormatBufVararg(
				buffer.data + buffer.offset,
				LOG_BUFSIZE - buffer.offset - 3, str, args
			)) > 0)
				buffer.offset += ret;
		} else {
			if((ret = (cs_int32)String_Append(
				buffer.data + buffer.offset,
				LOG_BUFSIZE - buffer.offset - 3, str
			)) > 0)
				buffer.offset += ret;
		}

		for(cs_size i = 0; i < buffer.offset; i++) {
			if((buffer.data[i] == '&' || buffer.data[i] == '%') && buffer.data[i + 1] != '\0') {
				for(cs_size j = i + 2; i < buffer.offset; j++) {
					buffer.data[j - 2] = buffer.data[j];
					if(buffer.data[j] == '\0') break;
				}
				buffer.offset -= 2;
			}
		}

		buffer.data[buffer.offset++] = '\r';
		buffer.data[buffer.offset++] = '\n';
		buffer.data[buffer.offset] = '\0';

		if(Event_Call(EVT_ONLOG, &buffer)) {
			ConsoleIO_PrePrint();
			File_Write(buffer.data, buffer.offset, 1, stderr);
			File_Flush(stderr);
			ConsoleIO_AfterPrint();
		}

		Mutex_Unlock(logMutex);
	}
}

void Log_Error(cs_str str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(LOG_ERROR, str, &args);
	va_end(args);
}

void Log_Info(cs_str str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(LOG_INFO, str, &args);
	va_end(args);
}

void Log_Chat(cs_str str, ...) {
	Log_Print(LOG_CHAT, str, (va_list *)NULL);
}

void Log_Warn(cs_str str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(LOG_WARN, str, &args);
	va_end(args);
}

void Log_Debug(cs_str str, ...) {
	va_list args;
	va_start(args, str);
	Log_Print(LOG_DEBUG, str, &args);
	va_end(args);
}
