#include "core.h"
#include "str.h"
#include <stdlib.h>

char* String_FindSubstr(const char* str, const char* strsrch) {
	return strstr(str, strsrch);
}

bool String_CaselessCompare(const char* str1, const char* str2) {
#if defined(WINDOWS)
	return _stricmp(str1, str2) == 0;
#elif defined(POSIX)
	return strcasecmp(str1, str2) == 0;
#endif
}

bool String_Compare(const char* str1, const char* str2) {
	return strcmp(str1, str2) == 0;
}

int String_ToInt(const char* str) {
	return atoi(str);
}

float String_ToFloat(const char* str) {
	return (float)atof(str);
}

size_t String_Length(const char* str) {
	return strlen(str);
}

size_t String_Append(char* dst, size_t len, const char* src) {
	size_t end = String_Length(dst);
	return String_Copy(dst + end, len - end, src);
}

size_t String_Copy(char* dst, size_t len, const char* src) {
	size_t _len = len;

	while(_len > 1 && (*dst++ = *src++) != '\0')
		--_len;
	*dst = 0;

	return len - _len;
}

char* String_CopyUnsafe(char* dst, const char* src) {
	return strcpy(dst, src);
}

uint32_t String_FormatError(uint32_t code, char* buf, size_t buflen) {
#if defined(WINDOWS)
	int len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (uint32_t)buflen, NULL);
	if(len > 0) {
		for(int i = 0; i < len; i++) {
			if(buf[i] == '\r' || buf[i] == '\n')
				buf[i] = '\0';
		}
	}
#elif defined(POSIX)
	int len = String_Copy(buf, buflen, strerror(code));
#endif
	return len;
}

void String_FormatBufVararg(char* buf, size_t len, const char* str, va_list* args) {
#if defined(WINDOWS)
	vsprintf_s(buf, len, str,* args);
#elif defined(POSIX)
	vsnprintf(buf, len, str,* args);
#endif
}

void String_FormatBuf(char* buf, size_t len, const char* str, ...) {
	va_list args;
	va_start(args, str);
	String_FormatBufVararg(buf, len, str, &args);
	va_end(args);
}

const char* String_AllocCopy(const char* str) {
	char* ptr = Memory_Alloc(1, String_Length(str) + 1);
	String_CopyUnsafe(ptr, str);
	return (const char*)ptr;
}

const char* String_FromArgument(const char* args, int index) {
	if(!args || *args == '\0') return NULL;

	do {
		if(index > 0 && *args == ' ') --index;
		if(index == 0) return args;
	} while(*args++ != '\0');

	return NULL;
}

size_t String_GetArgument(const char* args, char* arg, size_t arrsz, int index) {
	if(!args) return 0;
	const char* tmp = arg;

	while(*args != '\0') {
		if(index > 0) {
			if(*args == ' ') --index;
			++args;
		} else {
			do {
				*arg++ = *args++;
			} while((size_t)(arg - tmp) < arrsz && *args != '\0' && *args != ' ');
			*arg = '\0';
			break;
		}
	}

	return (size_t)(arg - tmp);
}
