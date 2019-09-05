#include "core.h"
#include "str.h"

bool String_CaselessCompare(const char* str1, const char* str2) {
#if defined(WINDOWS)
	return _stricmp(str1, str2) == 0;
#elif defined(POSIX)
	return strcasecmp(str1, str2) == 0;
#endif
}

int String_ToInt(const char* str) {
	return atoi(str);
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
	size_t blen = len;

	while((*dst++ = *src++) != '\0' && blen > 2)
		--blen;

	*dst = 0;
	return len - blen;
}

char* String_CopyUnsafe(char* dst, const char* src) {
	return strcpy(dst, src);
}

uint String_FormatError(uint code, char* buf, uint buflen) {
#if defined(WINDOWS)
	int len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, buf, buflen, NULL);
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
	if(!ptr)
		return NULL;
	String_CopyUnsafe(ptr, str);
	return (const char*)ptr;
}

size_t String_GetArgument(const char* args, char* arg, size_t arrsz, int index) {
	if(!args || !arg || arrsz < 1) return 0;

	size_t argsize = 0;

	while(1) {
		if(index > 0) {
			if(*args == '\0')
				return 0;

			if(*args == ' ')
				--index;

			++args;
		} else {
			if(arrsz < argsize) {
				arg[argsize] = '\0';
				break;
			}
			arg[argsize] = args[argsize];
			if(args[argsize] == '\0')
				break;
			if(args[argsize] == ' ') {
				arg[argsize] = '\0';
				break;
			}
			++argsize;
		}
	}

	return argsize;
}
