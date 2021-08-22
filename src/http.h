#ifndef HTTP_H
#define HTTP_H
#include "core.h"

#define HTTP_USERAGENT "CServer/1.0"

#if defined(WINDOWS)
#include <wininet.h>
HINTERNET hInternet;

typedef struct _Http {
	cs_bool secure;
	HINTERNET conn, req;
} Http;
#elif defined(UNIX)
typedef void CURL;

#define CURLOPT_WRITEDATA 10000 + 1
#define CURLOPT_URL 10000 + 2
#define CURLOPT_USERAGENT 10000 + 18
#define CURLOPT_FOLLOWLOCATION 0 + 52
#define CURLOPT_WRITEFUNCTION 20000 + 11
#define CURLE_OK 0

typedef struct _Http {
	cs_bool secure;
	cs_str domain;
	size_t buflen, rsplen;
	cs_char *path, *buf;
	CURL *handle;
} Http;
#endif

cs_bool Http_Init(void);
void Http_Uninit(void);

API cs_bool Http_Open(Http *http, cs_str domain);
API cs_bool Http_Request(Http *http, cs_str url);
API cs_ulong Http_ReadResponse(Http *http, cs_char *buf, cs_ulong sz);
API void Http_Cleanup(Http *http);
#endif // HTTP_H
