#include "core.h"
#include "str.h"

bool String_CaselessCompare(const char* str1, const char* str2) {
#if defined(WINDOWS)
	return _stricmp(str1, str2) == 0;
#elif defined(POSIX)
	return stricmp(str1, str2) == 0;
#endif
}

bool String_Compare(const char* str1, const char* str2) {
	return strcmp(str1, str2) == 0;
}

size_t String_Length(const char* str) {
	return strlen(str);
}

size_t String_Copy(char* dst, size_t len, const char* src) {
	if(!dst) return 0;
	if(!src) return 0;

#if defined(WINDOWS)
	strcpy_s(dst, len, src);
#elif defined(POSIX)
	if(String_Length(src) < len)
		strcpy(dst, src);
	else
		return 0;
#endif

	return len;
}

char* String_CopyUnsafe(char* dst, const char* src) {
	return strcpy(dst, src);
}

uint String_FormatError(uint code, char* buf, uint buflen) {
	int len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, buf, buflen, NULL);
	if(len > 0) {
		for(int i = 0; i < len; i++) {
			if(Error_WinBuf[i] == '\r' || Error_WinBuf[i] == '\n')
				Error_WinBuf[i] = ' ';
		}
	} else {
		Error_Set(ET_SYS, GetLastError());
	}

	return len;
}

void String_FormatBufVararg(char* buf, size_t len, const char* str, va_list* args) {
#if defined(WINDOWS)
	vsprintf_s(buf, len, str, *args);
#elif defined(POSIX)
	vsnprintf(buf, len, str, *args);
#endif
}

void String_FormatBuf(char* buf, size_t len, const char* str, ...) {
	va_list args;
	va_start(args, str);
	vsprintf_s(buf, len, str, args);
	va_end(args);
}

const char* String_AllocCopy(const char* str) {
	char* ptr = Memory_Alloc(1, String_Length(str) + 1);
	if(!ptr)
		return NULL;
	String_CopyUnsafe(ptr, str);
	return (const char*)ptr;
}
