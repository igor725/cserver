#include "core.h"
#include "platform.h"
#include "str.h"
#include <string.h>
#include <stdlib.h>

cs_bool String_CaselessCompare(const char* str1, const char* str2) {
	cs_uint8 c1, c2;

	while(true) {
		c1 = *str1++, c2 = *str2++;
		if(c1 >= 'A' && c1 <= 'Z') c1 += 32;
		if(c2 >= 'A' && c2 <= 'Z') c2 += 32;
		if(c1 != c2) return false;
		if(c1 == '\0' && c2 == '\0') return true;
	}
}

cs_bool String_CaselessCompare2(const char* str1, const char* str2, cs_size len) {
	cs_uint8 c1, c2;

	while(len--) {
		c1 = *str1++, c2 = *str2++;
		if(c1 >= 'A' && c1 <= 'Z') c1 += 32;
		if(c2 >= 'A' && c2 <= 'Z') c2 += 32;
		if(c1 != c2) return false;
	}

	return true;
}

cs_bool String_Compare(const char* str1, const char* str2) {
	cs_uint8 c1, c2;

	while(true) {
		c1 = *str1++, c2 = *str2++;
		if(c1 != c2) return false;
		if(c1 == '\0' && c2 == '\0') return true;
	}
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
	const char* s;
	for (s = str; *s; ++s);
	return s - str;
}

cs_size String_Append(char* dst, cs_size len, const char* src) {
	cs_size curr_len = String_Length(dst);
	return String_Copy(dst + curr_len, len - curr_len, src);
}

cs_size String_Copy(char* dst, cs_size len, const char* src) {
	cs_size avail = len;

	while(avail > 1 && (*dst++ = *src++) != '\0') avail--;
	*dst = '\0';

	return len - avail;
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

char* String_FindSubstr(const char* str, const char* strsrch) {
	return strstr(str, strsrch);
}

const char* String_AllocCopy(const char* str) {
	cs_size len = String_Length(str) + 1;
	char* ptr = Memory_Alloc(1, len);
	String_Copy(ptr, len, str);
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
		if(index > 0) {
			if(*args++ == ' ') --index;
			if(*args == '\0') return 0;
		} else {
			do {
				*arg++ = *args++;
			} while(--avail > 1 && *args != '\0' && *args != ' ');
			*arg = '\0';
			break;
		}
	}

	return len - avail;
}

size_t String_SizeOfB64(size_t inlen) {
	size_t ret = inlen;

	if (inlen % 3 != 0)
		ret += 3 - (inlen % 3);
	ret /= 3;
	ret *= 4;

	return ret;
}

const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

cs_size String_ToB64(const cs_uint8* src, cs_size len, char* dst) {
	cs_size elen = String_SizeOfB64(len);
	dst[elen] = '\0';

	for (cs_size i = 0, j = 0; i < len; i += 3, j += 4) {
		cs_int32 v = src[i];
		v = i + 1 < len ? v << 8 | src[i + 1] : v << 8;
		v = i + 2 < len ? v << 8 | src[i + 2] : v << 8;

		dst[j] = b64chars[(v >> 18) & 0x3F];
		dst[j + 1] = b64chars[(v >> 12) & 0x3F];
		if (i + 1 < len) {
			dst[j + 2] = b64chars[(v >> 6) & 0x3F];
		} else {
			dst[j + 2] = '=';
		}
		if (i + 2 < len) {
			dst[j + 3] = b64chars[v & 0x3F];
		} else {
			dst[j + 3] = '=';
		}
	}

	return elen;
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
