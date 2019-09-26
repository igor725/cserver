#ifndef ERROR_H
#define ERROR_H
enum ErrorTypes {
	ET_NOERR = -1,
	ET_SERVER,
	ET_ZLIB,
	ET_SYS,
	ET_STR
};

enum ErrorCodes {
	EC_OK,
	EC_NULLPTR,
	EC_MAGIC,
	EC_WIUNKID,
	EC_CFGTYPE,
	EC_CFGEND,
	EC_ITERINITED,
	EC_DLLPLUGVER
};

#define ERR_FMT "%s: %d in function %s: %s/%s"
#define Error_Print2(etype, ecode, abort) \
Error_Print(etype, ecode, __FILE__, __LINE__, __func__); \
if(abort) { \
	Process_Exit(ecode); \
}
#define Error_PrintSys Error_Print2(ET_SYS, GetLastError(), false); \

API void Error_Print(int type, uint32_t code, const char* file, uint32_t line, const char* func);
#endif
