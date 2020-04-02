#include "core.h"
#include "str.h"
#include "log.h"
#include "lang.h"
#include "error.h"
#include <miniz.h>

cs_str const Strings[] = {
	"All ok.",
	"Invalid magic.",
	"File \"%s\" corrupted.",
	"Unexpected end of file \"%s\".",
	"Can't parse line %d from file \"%s\".",
	"Entry \"%s\" is not registred for \"%s\".",
	"Trying to get entry \"%s\" from file \"%s\" as \"%s\", but the variable has type \"%s\".",
	"Invalid IPv4 address passed to Socket_SetAddr."
};

#if defined(WINDOWS)
#include <dbghelp.h>

void Error_CallStack(void) {
	void *stack[16];
	cs_uint16 frames;
	SYMBOL_INFO symbol = {0};
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, true);

	frames = CaptureStackBackTrace(0, 16, stack, NULL);

	symbol.MaxNameLen = 255;
	symbol.SizeOfStruct = sizeof(SYMBOL_INFO);

	for(cs_int32 i = 0; i < frames; i++) {
		SymFromAddr(process, (uintptr_t)stack[i], 0, &symbol);
		if(i > 2) {
			Log_Debug(Lang_Get(Lang_DbgGrp, 0), symbol.Name, symbol.Address);
#if _MSC_VER
			IMAGEHLP_LINE line = {0};
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
			if(SymGetLineFromAddr(process, (uintptr_t)symbol.Address, NULL, &line)) {
				Log_Debug(Lang_Get(Lang_DbgGrp, 1), line.FileName, line.LineNumber);
			}
#endif
		}
		if(symbol.Name && String_Compare(symbol.Name, "main")) break;
	}
}
#elif defined(POSIX)
#ifndef __ANDROID__
#include <execinfo.h>

void Error_CallStack(void) {
	void *stack[16];
	cs_int32 frames = backtrace(stack, 16);

	for(cs_int32 i = 0; i < frames; i++) {
		Dl_info dli = {0};
		dladdr(stack[i], &dli);
		if(i > 2) {
			Log_Debug(Lang_Get(Lang_DbgGrp, 0), dli.dli_sname, dli.dli_saddr);
		}
		if(dli.dli_sname && String_Compare(dli.dli_sname, "main")) break;
	}
}
#else
void Error_CallStack(void) {
	Log_Error("Error_CallStack not implemented.");
}
#endif

#endif

static void getErrorStr(cs_int32 type, cs_uint32 code, char *errbuf, cs_size sz, va_list *args) {
	switch(type) {
		case ET_SERVER:
			if(!args)
				String_Copy(errbuf, sz, Strings[code]);
			else
				String_FormatBufVararg(errbuf, sz, Strings[code], args);
			break;
		case ET_ZLIB:
			String_Copy(errbuf, sz, zError(code));
			break;
		case ET_SYS:
			if(!String_FormatError(code, errbuf, sz, args)) {
				String_Copy(errbuf, sz, Lang_Get(Lang_ErrGrp, 0));
			}
			break;
	}
}

cs_int32 Error_GetSysCode(void) {
#if defined(WINDOWS)
	return GetLastError();
#elif defined(POSIX)
	return errno;
#endif
}

void Error_Print(cs_int32 type, cs_uint32 code, cs_str file, cs_uint32 line, cs_str func) {
	char strbuf[384] = {0};
	char errbuf[256] = {0};

	getErrorStr(type, code, errbuf, 256, NULL);
	if(String_FormatBuf(strbuf, 384, Lang_Get(Lang_ErrGrp, 1), file, line, func, errbuf)) {
		/*
		** Избегаем краша, если в строке ошибки по какой-то
		** причине остались форматируемые значения.
		*/
		Log_Error("%s", strbuf);
		Error_CallStack();
	}
}

void Error_PrintF(cs_int32 type, cs_uint32 code, cs_str file, cs_uint32 line, cs_str func, ...) {
	char strbuf[384] = {0};
	char errbuf[256] = {0};

	va_list args;
	va_start(args, func);
	getErrorStr(type, code, errbuf, 256, &args);
	va_end(args);
	if(String_FormatBuf(strbuf, 384, Lang_Get(Lang_ErrGrp, 1), file, line, func, errbuf)) {
		// См. комментарий в Error_Print
		Log_Error("%s", strbuf);
		Error_CallStack();
	}
}
