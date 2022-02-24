#include "core.h"
#ifdef CORE_USE_UNIX
#define _GNU_SOURCE
#endif
#include "str.h"
#include "cserror.h"
#include "compr.h"

#if defined(CORE_USE_WINDOWS)
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
		printf("Frame #%d: %s = %p\n", i, symbol->Name, (void *)symbol->Address);
		if(SymGetLineFromAddr(process, (cs_uintptr)symbol->Address, (void *)&stack[i], &line)) {
			printf("\tin %s at line %ld\n", line.FileName, line.LineNumber);
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
	printf("Callstack printing for android not implemented yet\n");
}

cs_bool Error_Init(void) {return true;}
void Error_Uninit(void) {}
#elif defined(CORE_USE_UNIX)
#include <dlfcn.h>
#include <execinfo.h>

static void PrintCallStack(void) {
	void *stack[16];
	cs_int32 frames = backtrace(stack, 16);

	for(cs_int32 i = 0; i < frames; i++) {
		Dl_info dli;
		if(dladdr(stack[i], &dli))
			printf("Frame #%d: %s = 0x%0X\n", i, dli.dli_sname, dli.dli_saddr);
	}
}

cs_bool Error_Init(void) {return true;}
void Error_Uninit(void) {}
#endif

cs_int32 Error_GetSysCode(void) {
#if defined(CORE_USE_WINDOWS)
	return GetLastError();
#elif defined(CORE_USE_UNIX)
	return errno;
#endif
}

void Error_Print(cs_int32 code, cs_str file, cs_uint32 line, cs_str func, ...) {
	cs_char strbuf[384];

	cs_int32 fmtpos = String_FormatBuf(strbuf, 384, "%s:%d in function %s: ", file, line, func);
	va_list args;
	va_start(args, func);
	String_FormatError(code, strbuf + fmtpos, 384 - fmtpos, &args);
	printf("%s\n", strbuf);
	va_end(args);
	PrintCallStack();
}
