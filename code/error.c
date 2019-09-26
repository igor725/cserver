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

void Error_CallStack() {

}

void Error_Print(int type, uint code, const char* file, uint line, const char* func) {
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
	if(String_Length(strbuf)) Log_Error(strbuf);
}
