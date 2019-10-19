#ifndef HTTP_H
#define HTTP_H
enum httpErrors {
	HTTP_ERR_OK,
	HTTP_ERR_INVALID_VERSION,
	HTTP_ERR_INVALID_CODE,
};
enum httpHeaderType {
	HDRT_INVALID,
	HDRT_STR,
	HDRT_INT,
};

typedef struct httpHeader {
	const char* key;
	int type;
	union {
		const char* vchar;
		uint32_t vint;
	} value;
	struct httpHeader* next;
} *HTTPHDR;

typedef struct httpResponse {
	bool readonly;
	int error;
	int code;
	HTTPHDR header;
} *HTTPRESP;

typedef struct httpRequest {
	struct sockaddr_in addr;
	const char* path;
	SOCKET sock;
	HTTPHDR header;
} *HTTPREQ;

API HTTPHDR HttpRequest_GetHeader(HTTPREQ req, const char* key);
API void HttpRequest_SetHeaderStr(HTTPREQ req, const char* key, const char* value);
API void HttpRequest_SetHeaderInt(HTTPREQ req, const char* key, int value);
API void HttpRequest_SetHost(HTTPREQ req, const char* host, uint16_t port);
API void HttpRequest_SetPath(HTTPREQ req, const char* path);
API void HttpRequest_Perform(HTTPREQ req, HTTPRESP resp);
API void HttpRequest_Cleanup(HTTPREQ req);
#endif
