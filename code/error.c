#include "core.h"
#include "str.h"
#include "error.h"
#include "lang.h"

const char* const Strings[] = {
	"All ok.",
	"Invalid magic.",
	"File \"%s\" corrupted.",
	"Unexpected end of file \"%s\".",
	"Can't parse line %d from file \"%s\".",
	"Entry \"%s\" is not registred for \"%s\".",
	"Trying to get entry \"%s\" from file \"%s\" as \"%s\", but the variable has type \"%s\".",
	"Iterator already inited.",
	"Invalid IPv4 address passed to Socket_SetAddr."
};

#if defined(WINDOWS)
#include <dbghelp.h>

void Error_CallStack(void) {
	void* stack[16];
	uint16_t frames;
	SYMBOL_INFO symbol = {0};
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, true);

	frames = CaptureStackBackTrace(0, 16, stack, NULL);

	symbol.MaxNameLen = 255;
	symbol.SizeOfStruct = sizeof(SYMBOL_INFO);

	for(int32_t i = 0; i < frames; i++) {
		SymFromAddr(process, (uintptr_t)stack[i], 0, &symbol);
		if(i > 2) {
			Log_Debug(Lang_Get(LANG_DBGSYM0), symbol.Name, symbol.Address);
#if _MSC_VER
			IMAGEHLP_LINE line = {0};
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
			if(SymGetLineFromAddr(process, (uintptr_t)symbol.Address, NULL, &line)) {
				Log_Debug(Lang_Get(LANG_DBGSYM1), line.FileName, line.LineNumber);
			}
#endif
		}
		if(symbol.Name && String_Compare(symbol.Name, "main")) break;
	}
}
#elif defined(POSIX)
#include <execinfo.h>

void Error_CallStack(void) {
	void* stack[16];
	int32_t frames = backtrace(stack, 16);

	for(int32_t i = 0; i < frames; i++) {
		Dl_info dli = {0};
		dladdr(stack[i], &dli);
		if(i > 2) {
			Log_Debug(Lang_Get(LANG_DBGSYM0), dli.dli_sname, dli.dli_saddr);
		}
		if(dli.dli_sname && String_Compare(dli.dli_sname, "main")) break;
	}
}
#endif

static void getErrorStr(int32_t type, uint32_t code, char* errbuf, size_t sz, va_list* args) {
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
				String_Copy(errbuf, sz, Lang_Get(LANG_UNKERR));
			}
			break;
	}
}

int32_t Error_GetSysCode(void) {
#if defined(WINDOWS)
	return GetLastError();
#elif defined(POSIX)
	return errno;
#endif
}

void Error_Print(int32_t type, uint32_t code, const char* file, uint32_t line, const char* func) {
	char strbuf[384] = {0};
	char errbuf[256] = {0};

	getErrorStr(type, code, errbuf, 256, NULL);
	if(String_FormatBuf(strbuf, 384, Lang_Get(LANG_ERRFMT), file, line, func, errbuf)) {
		/*
			Избегаем краша, если в строке ошибки по какой-то
			причине остались форматируемые значения.
		*/
		Log_Error("%s", strbuf);
		Error_CallStack();
	}
}

void Error_PrintF(int32_t type, uint32_t code, const char* file, uint32_t line, const char* func, ...) {
	char strbuf[384] = {0};
	char errbuf[256] = {0};

	va_list args;
	va_start(args, func);
	getErrorStr(type, code, errbuf, 256, &args);
	va_end(args);
	if(String_FormatBuf(strbuf, 384, Lang_Get(LANG_ERRFMT), file, line, func, errbuf)) {
		// См. комментарий в Error_Print
		Log_Error("%s", strbuf);
		Error_CallStack();
	}
}
