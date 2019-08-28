#ifndef ERROR_H
#define ERROR_H

enum ErrorTypes {
	ET_NOERR = -1,
	ET_SERVER = 0,
	ET_ZLIB,
	ET_SYS
};

#define LNUM ": "
#define Error_Set(etype, ecode) (Error_SetCode(__FILE__, __LINE__, __func__, etype, ecode))

int   Error_Type;
uint  Error_Code;
const char* Error_Func;
const char* Error_File;
int   Error_Line;
char Error_WinBuf[512];

enum ErrorCodes {
	EC_OK = 0,
	EC_MAGIC,
};

void Error_SetCode(const char* efile, int eline, const char* efunc, int etype, uint ecode);
const char* Error_GetString();
const char* Error_GetFunc();
const char* Error_GetFile();
void Error_SetSuccess();
#endif
