#ifndef STR_H
#define STR_H
#include "core.h"
#include <stdarg.h>

API cs_char *String_FindSubstr(cs_str str, cs_str strsrch);
API cs_bool String_Compare(cs_str str1, cs_str str2);
API cs_bool String_CaselessCompare(cs_str str1, cs_str str2);
API cs_bool String_CaselessCompare2(cs_str str1, cs_str str2, cs_size len);
API cs_size String_Length(cs_str str);
API cs_size String_Append(cs_char *dst, cs_size len, cs_str src);
API cs_char *String_Grow(cs_char *src, cs_size add, cs_size *new);
API cs_size String_Copy(cs_char *dst, cs_size len, cs_str src);
API cs_uint32 String_FormatError(cs_uint32 code, cs_char *buf, cs_size buflen, va_list *args);
API cs_int32 String_FormatBufVararg(cs_char *buf, cs_size len, cs_str str, va_list *args);
API cs_int32 String_FormatBuf(cs_char *buf, cs_size len, cs_str str, ...);
API cs_str String_LastChar(cs_str str, cs_char sym);
API cs_str String_FirstChar(cs_str str, cs_char sym);
API cs_str String_AllocCopy(cs_str str);
API cs_size String_GetArgument(cs_str args, cs_char *arg, cs_size len, cs_int32 index);
API cs_uint32 String_CountArguments(cs_str args);
API cs_str String_FromArgument(cs_str args, cs_int32 index);
API cs_int32 String_ToInt(cs_str str);
API cs_int32 String_HexToInt(cs_str str);
API cs_float String_ToFloat(cs_str str);
API cs_size String_SizeOfB64(cs_size inlen);
API cs_size String_ToB64(const cs_byte *src, cs_size len, cs_char *dst);
#endif // STR_H
