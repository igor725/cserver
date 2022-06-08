#include "core.h"
#include "platform.h"
#include "str.h"
#include <string.h>
#include <stdlib.h>

cs_bool String_CaselessCompare(cs_str str1, cs_str str2) {
	cs_byte c1, c2;

	while(true) {
		c1 = *str1++, c2 = *str2++;
		if(c1 == c2 && c2 == '\0') return true;
		if(c1 >= 'A' && c1 <= 'Z') c1 += 32;
		if(c2 >= 'A' && c2 <= 'Z') c2 += 32;
		if(c1 != c2) return false;
	}
}

cs_bool String_CaselessCompare2(cs_str str1, cs_str str2, cs_size len) {
	cs_byte c1, c2;

	while(len--) {
		c1 = *str1++, c2 = *str2++;
		if(c1 >= 'A' && c1 <= 'Z') c1 += 32;
		if(c2 >= 'A' && c2 <= 'Z') c2 += 32;
		if(c1 != c2) return false;
	}

	return true;
}

cs_bool String_Compare(cs_str str1, cs_str str2) {
	if(!str1 || !str2) return false;
	cs_byte c1, c2;

	while(true) {
		c1 = *str1++, c2 = *str2++;
		if(c1 != c2) return false;
		if(c1 == '\0' && c2 == '\0') return true;
	}
}

cs_int32 String_ToInt(cs_str str) {
	return atoi(str);
}

cs_int32 String_HexToInt(cs_str str) {
	return strtol(str, NULL, 16);
}

cs_float String_ToFloat(cs_str str) {
	return (cs_float)atof(str);
}

cs_size String_Length(cs_str str) {
	cs_str s;
	for (s = str; *s; ++s);
	return s - str;
}

cs_size String_Append(cs_char *dst, cs_size len, cs_str src) {
	cs_size curr_len = String_Length(dst);
	return String_Copy(dst + curr_len, len - curr_len, src);
}

cs_char *String_Grow(cs_char *src, cs_size add, cs_size *new) {
	cs_size newp = String_Length(src) + add + 1;
	if(new) *new = newp;
	return Memory_Realloc(src, newp);
}

cs_size String_Copy(cs_char *dst, cs_size len, cs_str src) {
	cs_size avail = len;

	while(avail > 1 && (*dst++ = *src++) != '\0') avail--;
	*dst = '\0';

	return len - avail;
}

cs_uint32 String_FormatError(cs_uint32 code, cs_char *buf, cs_size buflen, va_list *args) {
#if defined(CORE_USE_WINDOWS)
	cs_int32 len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (cs_uint32)buflen, args);
	if(len > 0) {
		while(*buf++ != '\0') {
			if(*buf == '\r' || *buf == '\n') {
				*buf = '\0';
				break;
			}
		}
	}
	return len;
#elif defined(CORE_USE_UNIX)
	(void)args;
	return String_Copy(buf, buflen, strerror(code));
#endif
}

cs_int32 String_FormatBufVararg(cs_char *buf, cs_size len, cs_str str, va_list *args) {
	return vsnprintf(buf, len, str, *args);
}

cs_int32 String_FormatBuf(cs_char *buf, cs_size len, cs_str str, ...) {
	cs_int32 wrlen;
	va_list args;
	va_start(args, str);
	wrlen = String_FormatBufVararg(buf, len, str, &args);
	va_end(args);
	return wrlen;
}

cs_char *String_LastChar(cs_str str, cs_char sym) {
	return strrchr(str, sym);
}

cs_char *String_FirstChar(cs_str str, cs_char sym) {
	return strchr(str, sym);
}

cs_char *String_FindSubstr(cs_str str, cs_str strsrch) {
	return strstr(str, strsrch);
}

cs_str String_TrimExtension(cs_str str) {
	cs_char *ext = String_LastChar(str, '.');
	if(ext) *ext = '\0';
	return str;
}

cs_str String_AllocCopy(cs_str str) {
	cs_size len = String_Length(str) + 1;
	cs_char *ptr = Memory_Alloc(1, len);
	String_Copy(ptr, len, str);
	return (cs_str)ptr;
}

cs_str String_FromArgument(cs_str args, cs_int32 index) {
	if(!args) return NULL;

	do {
		if(index > 0 && *args == ' ') {
			--index; ++args;
		}
		if(index == 0) return args;
	} while(*args++ != '\0');

	return NULL;
}

cs_size String_GetArgument(cs_str args, cs_char *arg, cs_size len, cs_int32 index) {
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

cs_uint32 String_CountArguments(cs_str args) {
	if(!args || *args == '\0') return 0;
	cs_uint32 cnt = 1;

	while(*args++ != '\0')
		if(*args == ' ') cnt++;

	return cnt;
}

cs_bool String_IsSafe(cs_str str) {
	for(cs_size i = 0; str[i] != '\0'; i++)
		if(str[i] == '.' || str[i] == '/' || str[i] == '\\') return false;
	return true;
}

cs_size String_SizeOfB64(cs_size inlen) {
	if (inlen % 3 != 0)
		inlen += 3 - (inlen % 3);
	inlen /= 3;
	inlen *= 4;
	return inlen;
}

static const cs_char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

cs_size String_ToB64(const cs_byte *src, cs_size len, cs_char *dst) {
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
