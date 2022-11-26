#include "core.h"
#ifdef CORE_USE_UNIX
#define _GNU_SOURCE
#endif
#include "str.h"
#include "cserror.h"
#include "platform.h"

#if defined(CORE_USE_WINDOWS)
#include <dbghelp.h>

NOINL static void PrintCallStack(void) {
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
		if(SymFromAddr(GetCurrentProcess(), (cs_uintptr)stack[i], NULL, symbol)) {
			printf("Frame #%d: %s = %p\n", i, symbol->Name, (void *)symbol->Address);
			if(SymGetLineFromAddr(GetCurrentProcess(), (cs_uintptr)symbol->Address, (void *)&stack[i], &line))
				printf("\tin %s:%ld\n", line.FileName, line.LineNumber);
			if(String_Compare(symbol->Name, "main")) break;
		}
	}
}

cs_bool Error_Init(void) {
	return SymInitialize(GetCurrentProcess(), NULL, true) != false;
}

void Error_Uninit(void) {
	SymCleanup(GetCurrentProcess());
}
#elif defined(CORE_USE_UNIX)
#include <dlfcn.h>
#if defined(CORE_USE_LINUX) || defined(CORE_USE_DARWIN)
#	include <execinfo.h>
#endif

static void PrintCallStack(void) {
#	if defined(CORE_USE_LINUX) || defined(CORE_USE_DARWIN)
		void *stack[16];
		cs_int32 frames = backtrace(stack, 16);

		for(cs_int32 i = 0; i < frames; i++) {
			Dl_info dli;
			if(dladdr(stack[i], &dli)) {
				printf("Frame #%d: %s = %p\n", i, dli.dli_sname, dli.dli_saddr);
				if(String_Compare(dli.dli_sname, "main")) break;
			}
		}
#	else
		printf("*** Callstack printing is not implemented yet for this OS\n");
#	endif
}

cs_bool Error_Init(void) {return true;}
void Error_Uninit(void) {}
#endif

void Error_Print(cs_bool abort, cs_int32 code, cs_str file, cs_uint32 line, cs_str func, ...) {
	cs_char strbuf[384];

	cs_int32 fmtpos = String_FormatBuf(strbuf, 384, "%s:%d in function %s: ", file, line, func);
	va_list args;
	va_start(args, func);
	String_FormatError(code, strbuf + fmtpos, 384 - fmtpos, &args);
	printf("%s\n", strbuf);
	va_end(args);
	PrintCallStack();

	if(abort) Process_Exit(code);
}
