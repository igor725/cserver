#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"

#if defined(WINDOWS)
cs_bool Http_Init(void) {
	hInternet = InternetOpenA(HTTP_USERAGENT,
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL, NULL, 0
	);
	return hInternet != NULL;
}

void Http_Uninit(void) {
	if(!hInternet) return;
	InternetCloseHandle(hInternet);
	hInternet = NULL;
}

cs_bool Http_Open(Http *http, cs_str domain) {
	if(!hInternet && !Http_Init()) return false;
	http->conn = InternetConnectA(
		hInternet, domain,
		http->secure ? 443 : 80,
		NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1
	);
	return http->conn != INVALID_HANDLE_VALUE;
}

cs_bool Http_Request(Http *http, cs_str url) {
	if(!hInternet) return false;
	http->req = HttpOpenRequestA(
		http->conn, "GET", url,
		NULL, NULL, 0, http->secure ? INTERNET_FLAG_SECURE : 0, 1
	);
	return HttpSendRequest(http->req, NULL, 0, NULL, 0) == true;
}

cs_ulong Http_ReadResponse(Http *http, cs_char *buf, cs_ulong sz) {
	if(!hInternet) return 0L;
	cs_ulong readed;
	if(InternetReadFile(http->req, buf, sz - 1, &readed)) {
		buf[readed] = '\0';
		return readed;
	}
	return 0L;
}

void Http_Cleanup(Http *http) {
	if(!hInternet) return;
	InternetCloseHandle(http->conn);
	InternetCloseHandle(http->req);
}
#elif defined(UNIX)
struct _CURLFuncs {
	void *lib;
	CURL *(*easy_init)(void);
	cs_int32 (*easy_setopt)(CURL *, cs_int32, ...);
	cs_int32 (*easy_perform)(CURL *);
	void (*easy_cleanup)(CURL *);
} curl;

cs_bool Http_Init(void) {
	if(curl.lib) return true;
	return (DLib_Load("libcurl.so.4", &curl.lib) || DLib_Load("libcurl.so.3", &curl.lib)) &&
	DLib_GetSym(curl.lib, "curl_easy_init", &curl.easy_init) &&
	DLib_GetSym(curl.lib, "curl_easy_setopt", &curl.easy_setopt) &&
	DLib_GetSym(curl.lib, "curl_easy_perform", &curl.easy_perform) &&
	DLib_GetSym(curl.lib, "curl_easy_cleanup", &curl.easy_cleanup);
}

void Http_Uninit(void) {
	if(!curl.lib) return;
	curl.easy_cleanup = NULL;
	curl.easy_init = NULL;
	curl.easy_perform = NULL;
	curl.easy_setopt = NULL;
	DLib_Unload(curl.lib);
	curl.lib = NULL;
}

cs_bool Http_Open(Http *http, cs_str domain) {
	if(!curl.lib && !Http_Init()) return false;
	http->domain = String_AllocCopy(domain);
	http->handle = curl.easy_init();
	if(http->handle) {
		curl.easy_setopt(http->handle, CURLOPT_USERAGENT, HTTP_USERAGENT);
		curl.easy_setopt(http->handle, CURLOPT_FOLLOWLOCATION, 1L);
		return true;
	}
	return false;
}

cs_bool Http_Request(Http *http, cs_str url) {
	if(!http->domain || !http->handle) return false;
	http->path = (cs_char *)(http->secure ? String_AllocCopy("https://") : String_AllocCopy("http://"));
	cs_size memsize;
	http->path = String_Grow(http->path, String_Length(http->domain) + String_Length(url), &memsize);
	String_Append(http->path, memsize, http->domain);
	String_Append(http->path, memsize, url);
	return curl.easy_setopt(http->handle, CURLOPT_URL, http->path) == CURLE_OK;
}

static cs_size writefunc(cs_char *ptr, cs_size sz, cs_size num, void *ud) {
	Http *http = (Http *)ud;
	cs_size full = sz * num,
	newlen = min(http->rsplen + full, http->buflen);

	if(http->rsplen < http->buflen) {
		Memory_Copy(http->buf + http->rsplen, ptr, newlen - http->rsplen - 1);
		http->rsplen = newlen;
	}

	return full;
}

cs_ulong Http_ReadResponse(Http *http, cs_char *buf, cs_ulong sz) {
	if(!http->handle) return false;
	http->buf = buf;
	http->buflen = sz;
	curl.easy_setopt(http->handle, CURLOPT_WRITEFUNCTION, writefunc);
	curl.easy_setopt(http->handle, CURLOPT_WRITEDATA, (void *)http);
	if(curl.easy_perform(http->handle) == CURLE_OK) {
		return http->rsplen;
	}
	return 0L;
}

void Http_Cleanup(Http *http) {
	if(!curl.easy_cleanup) return;
	if(http->domain) Memory_Free((void *)http->domain);
	if(http->path) Memory_Free((void *)http->path);
	if(http->handle) curl.easy_cleanup(http->handle);
	Memory_Zero(http, sizeof(Http));
}
#endif
