#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"

#if defined(WINDOWS)
void Http_Init(void) {
	hInternet = InternetOpenA("CServer/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
}

void Http_Uninit(void) {
	InternetCloseHandle(hInternet);
}

cs_bool Http_Open(Http *http, cs_str domain) {
	http->conn = InternetConnectA(
		hInternet, domain,
		http->https ? 443 : 80,
		NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1
	);
	return http->conn != INVALID_HANDLE_VALUE;
}

cs_bool Http_Request(Http *http, cs_str method, cs_str url) {
	http->req = HttpOpenRequestA(
		http->conn, method, url,
		NULL, NULL, 0, http->https ? INTERNET_FLAG_SECURE : 0, 1
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
}
#elif defined(POSIX)
// TODO: Реализовать curl запросы
#endif
