#ifndef HTTP_H
#define HTTP_H
enum {
	HTTP_ERR_OK,

	HTTP_ERROR_CONNECTION_FAILED,
	HTTP_ERR_RESPONSE_READ,

	HTTP_ERR_INVALID_VERSION,
	HTTP_ERR_INVALID_CODE,
	HTTP_ERR_INVALID_REQUEST,
	HTTP_ERR_TOO_BIG_BODY,
	HTTP_ERR_BODY_RECV_FAIL,
};
enum {
	HDRT_INVALID,
	HDRT_STR,
	HDRT_INT,
};

typedef struct httpHeader {
	const char* key;
	cs_int32 type;
	union {
		const char* vchar;
		cs_uint32 vint;
	} value;
	struct httpHeader* next;
} *HTTPHDR;

typedef struct httpResponse {
	cs_int32 error, code, bodysize;
	char* body;
	HTTPHDR header;
} *HTTPRESP;

typedef struct httpRequest {
	Socket sock;
	cs_int32 error;
	struct sockaddr_in addr;
	const char* path;
	HTTPHDR header;
} *HTTPREQ;

API const char* HttpCode_GetReason(cs_int32 code);

API HTTPHDR HttpRequest_GetHeader(HTTPREQ req, const char* key);
API void HttpRequest_SetHeaderStr(HTTPREQ req, const char* key, const char* value);
API const char* HttpRequest_GetHeaderStr(HTTPREQ req, const char* key);
API void HttpRequest_SetHeaderInt(HTTPREQ req, const char* key, cs_int32 value);
cs_int32 HttpRequest_GetHeaderInt(HTTPREQ req, const char* key);
API void HttpRequest_SetHost(HTTPREQ req, const char* host, cs_uint16 port);
API void HttpRequest_SetPath(HTTPREQ req, const char* path);
API cs_bool HttpRequest_Read(HTTPREQ req, Socket sock);
API cs_bool HttpRequest_Perform(HTTPREQ req, HTTPRESP resp);
API void HttpRequest_Cleanup(HTTPREQ req);

API HTTPHDR HttpResponse_GetHeader(HTTPRESP resp, const char* key);
API void HttpResponse_SetHeaderStr(HTTPRESP resp, const char* key, const char* value);
API const char* HttpResponse_GetHeaderStr(HTTPRESP resp, const char* key);
API void HttpResponse_SetHeaderInt(HTTPRESP resp, const char* key, cs_int32 value);
API cs_int32 HttpResponse_GetHeaderInt(HTTPRESP resp, const char* key);
API void HttpResponse_SetHeader(HTTPRESP resp, const char* key, const char* value);
API void HttpResponse_SetBody(HTTPRESP resp, char* body, cs_int32 size);
API cs_bool HttpResponse_SendTo(HTTPRESP resp, Socket sock);
API cs_bool HttpResponse_Read(HTTPRESP resp, Socket sock);
API void HttpResponse_Cleanup(HTTPRESP resp);
#endif
