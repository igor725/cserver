#ifndef STR_H
#define STR_H
#include <string.h>
#include <stdarg.h>

API char* String_FindSubstr(const char* str, const char* strsrch);
API bool String_Compare(const char* str1, const char* str2);
API bool String_CaselessCompare(const char* str1, const char* str2);
API size_t String_Length(const char* str);
API size_t String_Append(char* dst, size_t len, const char* src);
API size_t String_Copy(char* dst, size_t len, const char* src);
API char* String_CopyUnsafe(char* dst, const char* src);
API uint32_t String_FormatError(uint32_t code, char* buf, size_t buflen);
API void String_FormatBufVararg(char* buf, size_t len, const char* str, va_list* args);
API void String_FormatBuf(char* buf, size_t len, const char* str, ...);
API char* String_LastChar(char* str, char sym);
API const char* String_AllocCopy(const char* str);
API size_t String_GetArgument(const char* args, char* arg, size_t arrsz, int index);
API const char* String_FromArgument(const char* args, int index);
API int String_ToInt(const char* str);
API float String_ToFloat(const char* str);
#endif
