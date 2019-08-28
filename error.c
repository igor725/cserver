#include "core.h"
#include <zlib.h>
#include "error.h"

const char* const Error_Strings[] = {
	"All ok",
	"Invalid magic",
	"Unknown cfg entry type"
};

char Error_StrBuf[512] = {0};
int Error_Type = ET_SERVER;
uint Error_Code = 0;

const char* Error_GetString() {
	switch(Error_Type) {
		case ET_SERVER:
			return Error_Strings[Error_Code];
		case ET_ZLIB:
			return zError(Error_Code);
		case ET_SYS:
			if(String_FormatError(Error_Code, Error_StrBuf, 512) > 0)
				return Error_StrBuf;
			else
				return "[Unexpected error]";
		default:
			return Error_Strings[0];
	}
}

void Error_SetCode(const char* efile, int eline, const char* efunc, int etype, uint ecode) {
	Error_File = efile;
	Error_Line = eline;
	Error_Func = efunc;
	Error_Type = etype;
	Error_Code = ecode;
}

const char* Error_GetFunc() {
	if(Error_Func)
		return Error_Func;
	else
		return "[unknown function]";
}

const char* Error_GetFile() {
	if(Error_File)
		return Error_File;
	else
		return "[unknown file]";
}

void Error_SetSuccess() {
	Error_Type = ET_NOERR;
	Error_File = NULL;
	Error_Func = NULL;
	Error_Line = -1;
	Error_Code = 0;
}
