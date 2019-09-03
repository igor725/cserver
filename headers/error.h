#ifndef ERROR_H
#define ERROR_H
enum ErrorTypes {
	ET_NOERR = -1,
	ET_SERVER = 0,
	ET_ZLIB,
	ET_SYS,
	ET_STR
};

#define LNUM ": "
#define Error_Set(etype, ecode, abort) \
Error_SetCode(__FILE__, __LINE__, __func__, etype, ecode); \
if(abort) { \
	Log_FormattedError(); \
	Process_Exit(ecode); \
} \

int   Error_Type;
uint  Error_Code;
const char* Error_Func;
const char* Error_File;
int   Error_Line;

enum ErrorCodes {
	EC_OK = 0,
	EC_NULLPTR,
	EC_MAGIC,
	EC_WIUNKID,
	EC_CFGTYPE,
	EC_CFGEND,
	EC_ITERINITED
};

void Error_SetCode(const char* efile, int eline, const char* efunc, int etype, uint ecode);
void Error_SetStr_(const char* efile, int eline, const char* efunc, const char* estr);
const char* Error_GetString();
const char* Error_GetFunc();
const char* Error_GetFile();
const char* Error_GetType();
void Error_SetSuccess();
#endif
