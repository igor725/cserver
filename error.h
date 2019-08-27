#ifndef ERROR_H
#define ERROR_H

#define ET_NOERR -1
#define ET_SERVER 0
#define ET_ZLIB   1
#define ET_WIN    2

#define LNUM ": "
#define Error_Set(etype, ecode) (Error_SetCode(__FILE__, __LINE__, __func__, etype, ecode))

int   Error_Type;
int   Error_Code;
char* Error_Func;
char* Error_File;
int   Error_Line;
char Error_WinBuf[512];

enum ErrorCodes {
	EC_OK = 0,
};

void Error_SetCode(char* efile, int eline, char* efunc, int etype, int ecode);
const char* Error_GetString();
char* Error_GetFunc();
char* Error_GetFile();
void Error_SetSuccess();
#endif
