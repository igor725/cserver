#ifndef ERROR_H
#define ERROR_H

#define ET_NOERR -1
#define ET_SERVER 0
#define ET_IO     1
#define ET_ZLIB   2
#define ET_WIN    3

int   Error_Type;
int   Error_Code;
char* Error_Func;
char Error_WinBuf[512];

enum ErrorCodes {
	EC_OK = 0,
};

void Error_SetCode(int etype, int ecode, char* efunc);
const char* Error_GetString();
char* Error_GetFunc();
void Error_SetSuccess();
int Error_GetCode();
int Error_GetType();
#endif
