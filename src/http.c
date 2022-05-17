#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"

#if defined(HTTP_USE_WININET_BACKEND)
static cs_str wisymlist[] = {
	"InternetOpenA", "InternetConnectA",
	"HttpOpenRequestA", "HttpSendRequestA",
	"InternetReadFile", "InternetCloseHandle",
	NULL
};

struct _WinInet {
	void *lib;

	HINTERNET(__stdcall *IOpen)(cs_str, cs_ulong, cs_str, cs_str, cs_ulong);
	HINTERNET(__stdcall *IConnect)(HINTERNET, cs_str, INTERNET_PORT, cs_str, cs_str, cs_ulong, cs_ulong, cs_uintptr);
	HINTERNET(__stdcall *IOpenRequest)(HINTERNET, cs_str, cs_str, cs_str, cs_str, cs_str *, cs_ulong, cs_uintptr);
	BOOL(__stdcall *HSendRequest)(HINTERNET, cs_str, cs_ulong, void *, cs_ulong);
	BOOL(__stdcall *IReadFile)(HINTERNET, void *, cs_ulong, cs_ulong *);
	BOOL(__stdcall *IClose)(HINTERNET);
} WinInet;

static HINTERNET hInternet = NULL;

INL static cs_bool InitBackend(void) {
	if(!WinInet.lib && !DLib_LoadAll(DLib_List("wininet.dll"), wisymlist, (void **)&WinInet))
		return false;

	return (hInternet = WinInet.IOpen(HTTP_USERAGENT,
		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0
	)) != NULL;
}

void Http_Uninit(void) {
	if(!WinInet.lib) return;
	if(hInternet) {
		WinInet.IClose(hInternet);
		hInternet = NULL;
	}
	DLib_Unload(WinInet.lib);
	Memory_Zero(&WinInet, sizeof(WinInet));
}

cs_bool Http_Open(Http *http, cs_str domain) {
	if(!hInternet && !InitBackend()) return false;
	http->conn = WinInet.IConnect(
		hInternet, domain,
		http->secure ? 443 : 80,
		NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1
	);
	return http->conn != (HINTERNET)-1;
}

cs_bool Http_Request(Http *http, cs_str url) {
	if(!hInternet) return false;
	http->req = WinInet.IOpenRequest(
		http->conn, "GET", url,
		NULL, NULL, 0, http->secure ? INTERNET_FLAG_SECURE : 0, 1
	);
	if(!http->req) return false;
	return WinInet.HSendRequest(http->req, NULL, 0, NULL, 0) == true;
}

cs_ulong Http_ReadResponse(Http *http, cs_char *buf, cs_ulong sz) {
	cs_ulong readed = 0L;
	if(hInternet && WinInet.IReadFile(http->req, buf, sz - 1, &readed))
		buf[readed] = '\0';
	return readed;
}

void Http_Cleanup(Http *http) {
	if(!hInternet) return;
	if(http->conn) WinInet.IClose(http->conn);
	if(http->req) WinInet.IClose(http->req);
}
#elif defined(HTTP_USE_CURL_BACKEND)
static cs_str csymlist[] = {
	"curl_easy_init", "curl_easy_setopt",
	"curl_easy_perform", "curl_easy_cleanup",
	NULL
};

static cs_str libcurl[] = {
#if defined(CORE_USE_UNIX)
	"libcurl." DLIB_EXT ".4",
	"libcurl." DLIB_EXT ".3",
	"libcurl." DLIB_EXT,
#elif defined(CORE_USE_WINDOWS)
	"curl.dll",
	"libcurl.dll",
#else
#	error This file wants to be hacked
#endif
	NULL
};

static struct _cURLLib {
	void *lib;

	CURL *(*easy_init)(void);
	cs_int32 (*easy_setopt)(CURL *, cs_int32, ...);
	cs_int32 (*easy_perform)(CURL *);
	void (*easy_cleanup)(CURL *);
} curl;

void Http_Uninit(void) {
	if(!curl.lib) return;
	DLib_Unload(curl.lib);
	Memory_Zero(&curl, sizeof(curl));
}

cs_bool Http_Open(Http *http, cs_str domain) {
	if(!curl.lib && !DLib_LoadAll(libcurl, csymlist, (void **)&curl))
		return false;
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
	if(!curl.lib || !http->domain || !http->handle) return false;
	http->path = (cs_char *)(http->secure ? String_AllocCopy("https://") : String_AllocCopy("http://"));
	cs_size memsize = 0;
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
	if(!curl.lib || !http->handle) return false;
	http->buf = buf, http->buflen = sz;
	curl.easy_setopt(http->handle, CURLOPT_WRITEFUNCTION, writefunc);
	curl.easy_setopt(http->handle, CURLOPT_WRITEDATA, (void *)http);
	if(curl.easy_perform(http->handle) == CURLE_OK)
		return (cs_ulong)http->rsplen;
	return 0L;
}

void Http_Cleanup(Http *http) {
	if(!curl.lib) return;
	if(http->domain) Memory_Free((void *)http->domain);
	if(http->path) Memory_Free((void *)http->path);
	if(http->handle) curl.easy_cleanup(http->handle);
	Memory_Zero(http, sizeof(Http));
}
#else
#	error No HTTP backend selected!
#endif
