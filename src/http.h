#ifndef HTTP_H
#define HTTP_H
#define HTTP_USERAGENT "CServer/1.0"

#if defined(WINDOWS)
#include <wininet.h>
HINTERNET hInternet;

typedef struct {
	cs_bool secure;
	HINTERNET conn, req;
} Http;
#elif defined(POSIX)
#include <curl/curl.h>

typedef struct {
	cs_bool secure;
	cs_str domain;
	size_t buflen, rsplen;
	char *path, *buf;
	CURL *handle;
} Http;
#endif

void Http_Init(void);
void Http_Uninit(void);

API cs_bool Http_Open(Http *http, cs_str domain);
API cs_bool Http_Request(Http *http, cs_str url);
API cs_ulong Http_ReadResponse(Http *http, char *buf, cs_ulong sz);
API void Http_Cleanup(Http *http);
#endif // HTTP_H
