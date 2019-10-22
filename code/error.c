#include "core.h"
#include "str.h"
#include "error.h"

const char* const Strings[] = {
	"All ok.",
	"Pointer is NULL.",
	"Invalid magic.",
	"World \"%s\" corrupted.",
	"Unknown cfg entry type in file \"%s\": \"%c\" - is not a valid variable type.",
	"Unexpected end of cfg file \"%s\".",
	"Config entry \"%s\" is not registred for \"%s\".",
	"Trying to get cfg entry \"%s\" from file \"%s\" as \"%s\", but the variable has type \"%s\".",
	"Iterator already inited.",
	"Invalid C-plugin version."
};

#define SYM_DBG "Symbol: %s - 0x%0X"

#if defined(WINDOWS)
#include <dbghelp.h>
void Error_CallStack(void) {
	void* stack[64];
	uint16_t frames;
	SYMBOL_INFO symbol = {0};
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, true);

	frames = CaptureStackBackTrace(0, 64, stack, NULL);

	symbol.MaxNameLen = 255;
	symbol.SizeOfStruct = sizeof(SYMBOL_INFO);

	for(int i = 0; i < frames; i++) {
		SymFromAddr(process, (uintptr_t)stack[i], 0, &symbol);
		if(i > 2) {
			Log_Debug(SYM_DBG, symbol.Name, symbol.Address);
#if _MSC_VER
			IMAGEHLP_LINE line = {0};
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
			if(SymGetLineFromAddr(process, (uintptr_t)symbol.Address, NULL, &line)) {
				Log_Debug("\tFile: %s: %d", line.FileName, line.LineNumber);
			}
#endif
		}
		if(symbol.Name && String_Compare(symbol.Name, "main")) break;
	}
}
#elif defined(POSIX)
#include <execinfo.h>
void Error_CallStack(void) {
	void* stack[64];
	int frames = backtrace(stack, 64);

	for(int i = 0; i < frames; i++) {
		Dl_info dli = {0};
		dladdr(stack[i], &dli);
		if(i > 2) {
			Log_Debug(SYM_DBG, dli.dli_sname, dli.dli_saddr);
		}
		if(dli.dli_sname && String_Compare(dli.dli_sname, "main")) break;
	}
}
#endif

static void getErrorStr(int type, uint32_t code, char* errbuf, size_t sz, va_list* args) {
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
				String_Copy(errbuf, sz, "Unexpected error");
			}
			break;
	}
}

void Error_Print(int type, uint32_t code, const char* file, uint32_t line, const char* func) {
	char strbuf[1024] = {0};
	char errbuf[512] = {0};

	getErrorStr(type, code, errbuf, 512, NULL);
	String_FormatBuf(strbuf, 1024, ERR_FMT, file, line, func, errbuf);
	if(String_Length(strbuf)) {
		/*
			Избегаем ошибки, если в строке ошибки по какой-то
			причине остались форматируемые значения.
		*/
		Log_Error("%s", strbuf);
		Error_CallStack();
	}
}

void Error_PrintF(int type, uint32_t code, const char* file, uint32_t line, const char* func, ...) {
	char strbuf[1024] = {0};
	char errbuf[512] = {0};

	va_list args;
	va_start(args, func);
	getErrorStr(type, code, errbuf, 512, &args);
	va_end(args);
	String_FormatBuf(strbuf, 1024, ERR_FMT, file, line, func, errbuf);
	if(String_Length(strbuf)) {
		Log_Error(strbuf);
		Error_CallStack();
	}
}
