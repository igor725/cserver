#include "core.h"
#include "platform.h"
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

bool String_CaselessCompare2(const char* str1, const char* str2, size_t count) {
#if defined(WINDOWS)
	return _memicmp((void*)str1, (void*)str2, count) == 0;
#elif defined(POSIX)
	return memicmp((void*)str1, (void*)str2, count) == 0;
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

	while(_len > 1 && (*dst++ = *src++) != '\0') --_len;
	*dst = 0;

	return len - _len;
}

char* String_CopyUnsafe(char* dst, const char* src) {
	return strcpy(dst, src);
}

uint32_t String_FormatError(uint32_t code, char* buf, size_t buflen, va_list* args) {
#if defined(WINDOWS)
	int len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (uint32_t)buflen, args);
	if(len > 0) {
		while(*buf++ != '\0') {
			if(*buf == '\r' || *buf == '\n') {
				*buf = '\0';
				break;
			}
		}
	}
	return len;
#elif defined(POSIX)
	return String_Copy(buf, buflen, strerror(code));
#endif
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

const char* String_LastChar(const char* str, char sym) {
	return strrchr(str, sym);
}

const char* String_FirstChar(const char* str, char sym) {
	return strchr(str, sym);
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
	if(!args || arrsz == 0) return 0;
	size_t start_len = arrsz;

	while(*args != '\0') {
		if(index > 0) {
			if(*args++ == ' ') --index;
			if(*args == '\0') return 0;
		} else {
			do {
				*arg++ = *args++;
			} while(--arrsz > 1 && *args != '\0' && *args != ' ');
			*arg = '\0';
			break;
		}
	}

	return start_len - arrsz;
}

/*
	Взято здеся:
	https://stackoverflow.com/questions/21001659/crc32-algorithm-implementation-in-c-without-a-look-up-table-and-with-a-public-li
*/
uint32_t String_CRC32(const uint8_t* str) {
	int i, j;
	uint32_t byte, crc, mask;

	i = 0;
	crc = 0xFFFFFFFF;
	while (str[i] != 0) {
		byte = str[i];
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {
			mask = ~((crc & 1) - 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
		i = i + 1;
	}
	return ~crc;
}
