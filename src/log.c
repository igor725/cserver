#include "core.h"
#include "str.h"
#include "platform.h"
#include "log.h"
#include "event.h"
#include "consoleio.h"

cs_byte Log_Flags = LOG_ALL;
static Mutex *logMutex = NULL;

#define MKCOL(c) "\x1B["c"m"
#define MKTCOL(c, t) Log_Flags&LOG_COLORS?MKCOL(c)t"\x1B[0m":t

static cs_str MapColor(cs_char col) {
	switch(col) {
		case '0': return MKCOL("30");
		case '1': return MKCOL("34");
		case '2': return MKCOL("32");
		case '3': return MKCOL("36");
		case '4': return MKCOL("31");
		case '5': return MKCOL("35");
		case '6': return MKCOL("2;33");
		case '7': return MKCOL("1;30");
		case '8': return MKCOL("2;37");
		case '9': return MKCOL("1;34");
		case 'a': return MKCOL("1;32");
		case 'b': return MKCOL("1;34");
		case 'c': return MKCOL("1;31");
		case 'd': return MKCOL("1;35");
		case 'e': return MKCOL("33");
		case 'f': return MKCOL("0");
	}
	return NULL;
}

INL static cs_str GetName(cs_byte flag) {
	switch(flag) {
		case LOG_ERROR:
			return MKTCOL("1;31", "ERROR");
		case LOG_INFO:
			return MKTCOL("1;32", "INFO ");
		case LOG_CHAT:
			return MKTCOL("1;33", "CHAT ");
		case LOG_WARN:
			return MKTCOL("35", "WARN ");
		case LOG_DEBUG:
			return MKTCOL("1;34", "DEBUG");
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

// Сдвигает все символы в лог буфере на указанное количество байт влево
INL static void BufShiftLeft(cs_size from, cs_size shift) {
	for(cs_size i = from + shift; i < buffer.offset; i++) {
		buffer.data[i - shift] = buffer.data[i];
		if(buffer.data[i] == '\0') break;
	}
	buffer.offset -= shift;
}

// Сдвигает все символы в лог буфере на указанное количество байт вправо
INL static cs_bool BufShiftRight(cs_size from, cs_size shift) {
	if(shift == 0) return false;
	for(cs_size i = buffer.offset; i >= from; i--) {
		if(LOG_BUFSIZE > i + shift)
			buffer.data[i + shift] = buffer.data[i];
		buffer.data[i] = '\0';
	}
	buffer.offset += shift;
	return true;
}

void Log_Print(cs_byte flag, cs_str str, va_list *args) {
	if(Log_Flags & flag) {
		Mutex_Lock(logMutex);

		cs_int32 ret;
		if((ret = Time_Format(buffer.data, LOG_BUFSIZE)) > 0)
			buffer.offset = ret;

		if((ret = String_FormatBuf(buffer.data + buffer.offset,
			LOG_BUFSIZE - buffer.offset, " [%s] ", GetName(flag)
		)) > 0)
			buffer.offset += ret;

		if(args) {
			if((ret = String_FormatBufVararg(
				buffer.data + buffer.offset,
				LOG_BUFSIZE - buffer.offset, str, args
			)) > 0)
				buffer.offset += ret;
			else goto logend;
		} else {
			if((ret = (cs_int32)String_Append(
				buffer.data + buffer.offset,
				LOG_BUFSIZE - buffer.offset, str
			)) > 0)
				buffer.offset += ret;
			else goto logend;
		}

		cs_char lastcolor = '\0';
		for(cs_size i = 0; i < buffer.offset; i++) {
			if((buffer.data[i] == '&' || buffer.data[i] == '%') && ISHEX(buffer.data[i + 1])) {
				cs_char currcol = buffer.data[i + 1];
				if(Log_Flags & LOG_COLORS && lastcolor != currcol) {
					cs_str color = MapColor(currcol);
					if(color) {
						cs_size clen = String_Length(color);
						if(clen == 0) continue;
						if(clen > 2) BufShiftRight(i + 2, clen - 2);
						Memory_Copy(buffer.data + i, color, clen);
						lastcolor = currcol;
						// Отнимаем 1 потому что цикл и так заинкрементит значение i
						i += clen - 1;
						continue;
					}
				}
				BufShiftLeft(i, 2);
			}
		}

		// Убеждаемся, что завершающие символы у нас поместятся в буфер
		buffer.offset = min(buffer.offset, LOG_BUFSIZE - 8);
		if(Log_Flags & LOG_COLORS)
			buffer.offset += String_Copy(
				buffer.data + buffer.offset,
				LOG_BUFSIZE - buffer.offset,
				MapColor('f')
			);
		buffer.data[buffer.offset++] = '\r';
		buffer.data[buffer.offset++] = '\n';
		buffer.data[buffer.offset] = '\0';

		if(Event_Call(EVT_ONLOG, &buffer)) {
			ConsoleIO_PrePrint();
			File_Write(buffer.data, buffer.offset, 1, stderr);
			File_Flush(stderr);
			ConsoleIO_AfterPrint();
		}

		logend:
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
