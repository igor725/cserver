#ifndef STR_H
#define STR_H
#include <stdarg.h>

API char* String_FindSubstr(const char* str, const char* strsrch);
API cs_bool String_Compare(const char* str1, const char* str2);
API cs_bool String_CaselessCompare(const char* str1, const char* str2);
API cs_bool String_CaselessCompare2(const char* str1, const char* str2, cs_size len);
API cs_size String_Length(const char* str);
API cs_size String_Append(char* dst, cs_size len, const char* src);
API cs_size String_Copy(char* dst, cs_size len, const char* src);
API char* String_CopyUnsafe(char* dst, const char* src);
API cs_uint32 String_FormatError(cs_uint32 code, char* buf, cs_size buflen, va_list* args);
API cs_int32 String_FormatBufVararg(char* buf, cs_size len, const char* str, va_list* args);
API cs_int32 String_FormatBuf(char* buf, cs_size len, const char* str, ...);
API const char* String_LastChar(const char* str, char sym);
API const char* String_FirstChar(const char* str, char sym);
API const char* String_AllocCopy(const char* str);
API cs_size String_GetArgument(const char* args, char* arg, cs_size arrsz, cs_int32 index);
API const char* String_FromArgument(const char* args, cs_int32 index);
cs_uint32 String_CRC32(const cs_uint8* str);
API cs_int32 String_ToInt(const char* str);
API cs_int32 String_HexToInt(const char* str);
API float String_ToFloat(const char* str);
#endif
