#include "core.h"
#include "str.h"
#include "platform.h"
#include "log.h"
#include "event.h"

cs_byte Log_Level = LOG_ALL;
Mutex *Log_Mutex;

INL static cs_str getName(cs_byte flag) {
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

cs_bool Log_Init(void) {
	return (Log_Mutex = Mutex_Create()) != NULL;
}

void Log_Uninit(void) {
	if(Log_Mutex) Mutex_Free(Log_Mutex);
}

void Log_SetLevelStr(cs_str str) {
	cs_byte level = LOG_ERROR;

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
				level = LOG_QUIET;
				break;
		}
	} while(*str++ != '\0');

	Log_Level = level;
}

static LogBuffer buffer;

void Log_Print(cs_byte flag, cs_str str, va_list *args) {
	if(Log_Level & flag) {
		Mutex_Lock(Log_Mutex);

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

		buffer.data[buffer.offset++] = '\r';
		buffer.data[buffer.offset++] = '\n';
		buffer.data[buffer.offset] = '\0';

		if(Event_Call(EVT_ONLOG, &buffer)) {
			File_Write(buffer.data, buffer.offset, 1, stderr);
			File_Flush(stderr);
		}

		Mutex_Unlock(Log_Mutex);
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
