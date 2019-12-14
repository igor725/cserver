#include "core.h"
#include "platform.h"
#include "str.h"
#include <string.h>
#include <stdlib.h>

char* String_FindSubstr(const char* str, const char* strsrch) {
	return strstr(str, strsrch);
}

cs_bool String_CaselessCompare(const char* str1, const char* str2) {
#if defined(WINDOWS)
	return _stricmp(str1, str2) == 0;
#elif defined(POSIX)
	return strcasecmp(str1, str2) == 0;
#endif
}

cs_bool String_CaselessCompare2(const char* str1, const char* str2, cs_size len) {
	cs_uint8 c1, c2;

	while(len-- > 0) {
		c1 = *str1++;
		c2 = *str2++;
		if(c1 >= 'A' && c1 <= 'Z') c1 += 32;
		if(c2 >= 'A' && c2 <= 'Z') c2 += 32;
		if(c1 != c2) return false;
	}

	return true;
}

cs_bool String_Compare(const char* str1, const char* str2) {
	return strcmp(str1, str2) == 0;
}

cs_int32 String_ToInt(const char* str) {
	return atoi(str);
}

cs_int32 String_HexToInt(const char* str) {
	return strtol(str, NULL, 16);
}

float String_ToFloat(const char* str) {
	return (float)atof(str);
}

cs_size String_Length(const char* str) {
	return strlen(str);
}

cs_size String_Append(char* dst, cs_size len, const char* src) {
	cs_size curr_len = String_Length(dst);
	return String_Copy(dst + curr_len, len - curr_len, src);
}

cs_size String_Copy(char* dst, cs_size len, const char* src) {
	cs_size avail = len;

	while(avail > 1 && (*dst++ = *src++) != '\0') --avail;
	*dst = '\0';

	return len - avail;
}

char* String_CopyUnsafe(char* dst, const char* src) {
	return strcpy(dst, src);
}

cs_uint32 String_FormatError(cs_uint32 code, char* buf, cs_size buflen, va_list* args) {
#if defined(WINDOWS)
	cs_int32 len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (cs_uint32)buflen, args);
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
	(void)args;
	return String_Copy(buf, buflen, strerror(code));
#endif
}

cs_int32 String_FormatBufVararg(char* buf, cs_size len, const char* str, va_list* args) {
#if defined(WINDOWS)
	return vsprintf_s(buf, len, str, *args);
#elif defined(POSIX)
	return vsnprintf(buf, len, str, *args);
#endif
}

cs_int32 String_FormatBuf(char* buf, cs_size len, const char* str, ...) {
	cs_int32 wrlen;
	va_list args;
	va_start(args, str);
	wrlen = String_FormatBufVararg(buf, len, str, &args);
	va_end(args);
	return wrlen;
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

const char* String_FromArgument(const char* args, cs_int32 index) {
	if(!args) return NULL;

	do {
		if(index > 0 && *args == ' ') --index;
		if(index == 0) return args;
	} while(*args++ != '\0');

	return NULL;
}

cs_size String_GetArgument(const char* args, char* arg, cs_size len, cs_int32 index) {
	if(len == 0 || args == NULL) return 0;
	cs_size avail = len;

	while(*args != '\0') {
		if(index > 0 && *args++ == ' ')
			--index;
		else {
			do {
				*arg++ = *args++;
			} while(--avail > 1 && *args != '\0' && *args != ' ');
			*arg = '\0';
			break;
		}
	}

	return len - avail;
}

/*
** Взято здеся:
** https://stackoverflow.com/questions/21001659/crc32-algorithm-implementation-in-c-without-a-look-up-table-and-with-a-public-li
*/
cs_uint32 String_CRC32(const cs_uint8* str) {
	cs_int32 i, j;
	cs_uint32 byte, crc, mask;

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
