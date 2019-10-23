#ifndef HTTP_H
#define HTTP_H
enum httpErrors {
	HTTP_ERR_OK,

	HTTP_ERR_RESPONSE_READ,

	HTTP_ERR_INVALID_VERSION,
	HTTP_ERR_INVALID_CODE,
	HTTP_ERR_INVALID_REQUEST,
	HTTP_ERR_TOO_BIG_BODY,
	HTTP_ERR_BODY_RECV_FAIL,
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
	int error, code, bodysize;
	char* body;
	HTTPHDR header;
} *HTTPRESP;

typedef struct httpRequest {
	SOCKET sock;
	int error;
	struct sockaddr_in addr;
	const char* path;
	HTTPHDR header;
} *HTTPREQ;

API const char* HttpCode_GetReason(int code);

API HTTPHDR HttpRequest_GetHeader(HTTPREQ req, const char* key);
API void HttpRequest_SetHeaderStr(HTTPREQ req, const char* key, const char* value);
API const char* HttpRequest_GetHeaderStr(HTTPREQ req, const char* key);
API void HttpRequest_SetHeaderInt(HTTPREQ req, const char* key, int value);
int HttpRequest_GetHeaderInt(HTTPREQ req, const char* key);
API void HttpRequest_SetHost(HTTPREQ req, const char* host, uint16_t port);
API void HttpRequest_SetPath(HTTPREQ req, const char* path);
API bool HttpRequest_Read(HTTPREQ req, SOCKET sock);
API bool HttpRequest_Perform(HTTPREQ req, HTTPRESP resp);
API void HttpRequest_Cleanup(HTTPREQ req);

API HTTPHDR HttpResponse_GetHeader(HTTPRESP resp, const char* key);
API void HttpResponse_SetHeaderStr(HTTPRESP resp, const char* key, const char* value);
API const char* HttpResponse_GetHeaderStr(HTTPRESP resp, const char* key);
API void HttpResponse_SetHeaderInt(HTTPRESP resp, const char* key, int value);
API int HttpResponse_GetHeaderInt(HTTPRESP resp, const char* key);
API void HttpResponse_SetHeader(HTTPRESP resp, const char* key, const char* value);
API void HttpResponse_SetBody(HTTPRESP resp, char* body, int size);
API bool HttpResponse_SendTo(HTTPRESP resp, SOCKET sock);
API bool HttpResponse_Read(HTTPRESP resp, SOCKET sock);
API void HttpResponse_Cleanup(HTTPRESP resp);
#endif
