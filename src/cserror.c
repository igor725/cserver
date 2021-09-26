#include "core.h"
#ifdef UNIX
#define _GNU_SOURCE
#endif
#include "str.h"
#include "log.h"
#include "cserror.h"
#include "compr.h"

#if defined(WINDOWS)
#include <dbghelp.h>

NOINL static void PrintCallStack(void) {
	HANDLE process = GetCurrentProcess();
	void *stack[16];
	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
	PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
	symbol->MaxNameLen = MAX_SYM_NAME;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	cs_uint16 frames = CaptureStackBackTrace(2, 16, stack, NULL);
	IMAGEHLP_LINE line = {
		.SizeOfStruct = sizeof(IMAGEHLP_LINE)
	};

	for(cs_int32 i = 0; i < frames; i++) {
		SymFromAddr(process, (cs_uintptr)stack[i], NULL, symbol);
		Log_Debug("Frame #%d: %s = 0x%0X", i, symbol->Name, symbol->Address);
		if(SymGetLineFromAddr(process, (cs_uintptr)symbol->Address, (void *)&stack[i], &line)) {
			Log_Debug("\tin %s at line %d", line.FileName, line.LineNumber);
		}
	}
}

cs_bool Error_Init(void) {
	return SymInitialize(GetCurrentProcess(), NULL, true) != false;
}

void Error_Uninit(void) {
	SymCleanup(GetCurrentProcess());
}

#elif defined(__ANDROID__)
static void PrintCallStack(void) {
	Log_Debug("Callstack printing for android not implemented yet");
}

cs_bool Error_Init(void) {return true;}
void Error_Uninit(void) {}
#elif defined(UNIX)
#include <dlfcn.h>
#include <execinfo.h>

static void PrintCallStack(void) {
	void *stack[16];
	cs_int32 frames = backtrace(stack, 16);

	for(cs_int32 i = 0; i < frames; i++) {
		Dl_info dli;
		if(dladdr(stack[i], &dli))
			Log_Debug("Frame #%d: %s = 0x%0X", i, dli.dli_sname, dli.dli_saddr);
	}
}

cs_bool Error_Init(void) {return true;}
void Error_Uninit(void) {}
#endif

INL static void getErrorStr(cs_int32 type, cs_int32 code, cs_char *errbuf, cs_size sz, va_list *args) {
	switch(type) {
		case ET_ZLIB:
			String_Copy(errbuf, sz, Compr_GetError(code));
			break;
		case ET_SYS:
			if(!String_FormatError(code, errbuf, sz, args)) {
				String_Copy(errbuf, sz, "Unexpected error.");
			}
			break;
	}
}

cs_int32 Error_GetSysCode(void) {
#if defined(WINDOWS)
	return GetLastError();
#elif defined(UNIX)
	return errno;
#endif
}

void Error_Print(cs_int32 type, cs_int32 code, cs_str file, cs_uint32 line, cs_str func, ...) {
	cs_char strbuf[384], errbuf[256];

	va_list args;
	va_start(args, func);
	getErrorStr(type, code, errbuf, 256, &args);
	va_end(args);
	if(String_FormatBuf(strbuf, 384, "%s:%d in function %s: %s", file, line, func, errbuf)) {
		Log_Error("%s", strbuf);
		PrintCallStack();
	}
}
