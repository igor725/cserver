#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"

#if defined(WINDOWS)
void Http_Init(void) {
	hInternet = InternetOpenA(HTTP_USERAGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
}

void Http_Uninit(void) {
	InternetCloseHandle(hInternet);
}

cs_bool Http_Open(Http *http, cs_str domain) {
	http->conn = InternetConnectA(
		hInternet, domain,
		http->secure ? 443 : 80,
		NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1
	);
	return http->conn != INVALID_HANDLE_VALUE;
}

cs_bool Http_Request(Http *http, cs_str url) {
	http->req = HttpOpenRequestA(
		http->conn, "GET", url,
		NULL, NULL, 0, http->secure ? INTERNET_FLAG_SECURE : 0, 1
	);
	return HttpSendRequest(http->req, NULL, 0, NULL, 0) == true;
}

cs_ulong Http_ReadResponse(Http *http, char *buf, cs_ulong sz) {
	cs_ulong readed;
	if(InternetReadFile(http->req, buf, sz - 1, &readed)) {
		buf[readed] = '\0';
		return readed;
	}
	return 0;
}

void Http_Cleanup(Http *http) {
	InternetCloseHandle(http->conn);
	InternetCloseHandle(http->req);
}
#elif defined(POSIX)
void Http_Init(void) {}
void Http_Uninit(void) {}

cs_bool Http_Open(Http *http, cs_str domain) {
	http->domain = String_AllocCopy(domain);
	http->handle = curl_easy_init();
	if(http->handle) {
		curl_easy_setopt(http->handle, CURLOPT_USERAGENT, HTTP_USERAGENT);
		curl_easy_setopt(http->handle, CURLOPT_FOLLOWLOCATION, 1L);
		return true;
	}
	return false;
}

cs_bool Http_Request(Http *http, cs_str url) {
	http->path = (char *)(http->secure ? String_AllocCopy("https://") : String_AllocCopy("http://"));
	cs_size memsize;
	http->path = String_Grow(http->path, String_Length(http->domain) + String_Length(url), &memsize);
	String_Append(http->path, memsize, http->domain);
	String_Append(http->path, memsize, url);
	return curl_easy_setopt(http->handle, CURLOPT_URL, http->path) == CURLE_OK;
}

static cs_size writefunc(char *ptr, cs_size sz, cs_size num, void *ud) {
	Http *http = (Http *)ud;
	cs_size full = sz * num,
	newlen = min(http->rsplen + full, http->buflen);

	if(http->rsplen < http->buflen) {
		Memory_Copy(http->buf + http->rsplen, ptr, newlen - http->rsplen - 1);
		http->rsplen = newlen;
	}

	return full;
}

cs_ulong Http_ReadResponse(Http *http, char *buf, cs_ulong sz) {
	http->buf = buf;
	http->buflen = sz;
	curl_easy_setopt(http->handle, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(http->handle, CURLOPT_WRITEDATA, (void *)http);
	if(curl_easy_perform(http->handle) == CURLE_OK) {
		return http->rsplen;
	}
	return 0L;
}

void Http_Cleanup(Http *http) {
	if(http->domain) Memory_Free((void *)http->domain);
	if(http->path) Memory_Free((void *)http->path);
	if(http->handle) curl_easy_cleanup(http->handle);
	Memory_Zero(http, sizeof(Http));
}
#endif
