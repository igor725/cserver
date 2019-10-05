#include "core.h"
#include "error.h"

const char* const Types[] = {
	"SERVER",
	"ZLIB",
	"SYSTEM"
};

const char* const Strings[] = {
	"All ok",
	"Pointer is NULL",
	"Invalid magic",
	"Unknown data type ID",
	"Unknown cfg entry type",
	"Unexpected end of cfg file",
	"Iterator already inited",
	"Invalid C-plugin version"
};

#define SYM_DBG "Symbol: %s - 0x%0X"

#if defined(WINDOWS)
#include <dbghelp.h>
void Error_CallStack(void) {
	if(Log_GetLevel() < LOG_DEBUG) {
		return;
	}
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
		if(String_Compare(symbol.Name, "main")) break; // Символы после main не несут смысловой нагрузки
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
		if(String_Compare(dli.dli_sname, "main")) break;
	}
}
#endif

void Error_Print(int type, uint32_t code, const char* file, uint32_t line, const char* func) {
	char strbuf[1024] = {0};
	char errbuf[512] = {0};

	switch(type) {
		case ET_SERVER:
			String_Copy(errbuf, 512, Strings[code]);
			break;
		case ET_ZLIB:
			String_Copy(errbuf, 512, zError(code));
			break;
		case ET_SYS:
			if(!String_FormatError(code, errbuf, 512)) {
				String_Copy(errbuf, 512, "Unexpected error");
			}
			break;
	}

	String_FormatBuf(strbuf, 1024, ERR_FMT, file, line, func, Types[type], errbuf);
	if(String_Length(strbuf)) {
		Log_Error(strbuf);
		Error_CallStack();
	}
}
