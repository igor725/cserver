#ifndef STR_H
#define STR_H
#include <stdarg.h>

API char *String_FindSubstr(cs_str str, cs_str strsrch);
API cs_bool String_Compare(cs_str str1, cs_str str2);
API cs_bool String_CaselessCompare(cs_str str1, cs_str str2);
API cs_bool String_CaselessCompare2(cs_str str1, cs_str str2, cs_size len);
API cs_size String_Length(cs_str str);
API cs_size String_Append(char *dst, cs_size len, cs_str src);
API char *String_Grow(char *src, cs_size add, cs_size *new);
API cs_size String_Copy(char *dst, cs_size len, cs_str src);
API cs_uint32 String_FormatError(cs_uint32 code, char *buf, cs_size buflen, va_list *args);
API cs_int32 String_FormatBufVararg(char *buf, cs_size len, cs_str str, va_list *args);
API cs_int32 String_FormatBuf(char *buf, cs_size len, cs_str str, ...);
API cs_str String_LastChar(cs_str str, char sym);
API cs_str String_FirstChar(cs_str str, char sym);
API cs_str String_AllocCopy(cs_str str);
API cs_size String_GetArgument(cs_str args, char *arg, cs_size len, cs_int32 index);
API cs_str String_FromArgument(cs_str args, cs_int32 index);
API cs_int32 String_ToInt(cs_str str);
API cs_int32 String_HexToInt(cs_str str);
API float String_ToFloat(cs_str str);
API cs_size String_SizeOfB64(cs_size inlen);
API cs_size String_ToB64(const cs_byte *src, cs_size len, char *dst);
#endif // STR_H
