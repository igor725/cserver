#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"

#if defined(HTTP_USE_WININET_BACKEND)
struct _WinInet {
	void *lib;

	HINTERNET(*IOpen)(cs_str, cs_ulong, cs_str, cs_str, cs_ulong);
	HINTERNET(*IConnect)(HINTERNET, cs_str, INTERNET_PORT, cs_str, cs_str, cs_ulong, cs_ulong, cs_uintptr);
	HINTERNET(*IOpenRequest)(HINTERNET, cs_str, cs_str, cs_str, cs_str, cs_str *, cs_ulong, cs_uintptr);
	HINTERNET(*IReadFile)(HINTERNET, void *, cs_ulong, cs_ulong *);
	BOOL(*IClose)(HINTERNET);

	BOOL(*HSendRequest)(HINTERNET, cs_str, cs_ulong, void *, cs_ulong);
} WinInet;

INL static cs_bool InitBackend(void) {
	if(!WinInet.lib) {
		if(!(DLib_Load("wininet.dll", &WinInet.lib) &&
			DLib_GetSym(WinInet.lib, "InternetOpenA", &WinInet.IOpen) &&
			DLib_GetSym(WinInet.lib, "InternetConnectA", &WinInet.IConnect) &&
			DLib_GetSym(WinInet.lib, "HttpOpenRequestA", &WinInet.IOpenRequest) &&
			DLib_GetSym(WinInet.lib, "InternetReadFile", &WinInet.IReadFile) &&
			DLib_GetSym(WinInet.lib, "InternetCloseHandle", &WinInet.IClose) &&
			DLib_GetSym(WinInet.lib, "HttpSendRequestA", &WinInet.HSendRequest)
		)) return false;
	} else if(hInternet) return true;
	hInternet = WinInet.IOpen(HTTP_USERAGENT,
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL, NULL, 0
	);
	return hInternet != NULL;
}

void Http_Uninit(void) {
	if(!hInternet) return;
	WinInet.IClose(hInternet);
	hInternet = NULL;
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
	if(!hInternet) return 0L;
	cs_ulong readed;
	if(WinInet.IReadFile(http->req, buf, sz - 1, &readed)) {
		buf[readed] = '\0';
		return readed;
	}
	return 0L;
}

void Http_Cleanup(Http *http) {
	if(!hInternet) return;
	if(http->conn) WinInet.IClose(http->conn);
	if(http->req) WinInet.IClose(http->req);
}
#elif defined(HTTP_USE_CURL_BACKEND)
#include "log.h"

#if defined(UNIX)
cs_str  libcurl = "libcurl.so.4",
libcurl_alt = "libcurl.so.3";
#elif defined(WINDOWS)
cs_str libcurl = "curl.dll",
libcurl_alt = "libcurl.dll";
#endif

struct _CURLFuncs {
	void *lib;
	CURL *(*easy_init)(void);
	cs_int32 (*easy_setopt)(CURL *, cs_int32, ...);
	cs_int32 (*easy_perform)(CURL *);
	void (*easy_cleanup)(CURL *);
} curl;

INL static cs_bool InitBackend(void) {
	if(curl.lib) return true;
	if(!(DLib_Load(libcurl, &curl.lib) || DLib_Load(libcurl_alt, &curl.lib))) {
		cs_char buf[512];
		Log_Info("libcurl loading failed: %s", DLib_GetError(buf, 512));
		return false;
	}
	return DLib_GetSym(curl.lib, "curl_easy_init", &curl.easy_init) &&
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
	if(!curl.lib && !InitBackend()) return false;
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
		return (cs_ulong)http->rsplen;
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
