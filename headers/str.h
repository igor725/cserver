#ifndef STR_H
#define STR_H
#include <string.h>
#include <stdarg.h>

API bool String_Compare(const char* str1, const char* str2);
API bool String_CaselessCompare(const char* str1, const char* str2);
API size_t String_Length(const char* str);
API size_t String_Copy(char* dst, size_t len, const char* src);
API char* String_CopyUnsafe(char* dst, const char* src);
API uint String_FormatError(uint code, char* buf, uint buflen);
API void String_FormatBufVararg(char* buf, size_t len, const char* str, va_list* args);
API void String_FormatBuf(char* buf, size_t len, const char* str, ...);
API const char* String_AllocCopy(const char* str);
API size_t String_GetArgument(const char* args, char* arg, size_t arrsz, int index);
API int String_ToInt(const char* str);
#endif
