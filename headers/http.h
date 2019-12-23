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

typedef struct HTTPHeader {
	const char* key;
	cs_int32 type;
	union {
		const char* vchar;
		cs_uint32 vint;
	} value;
	struct HTTPHeader* next;
} HTTPHeader;

typedef struct HTTPResponse {
	cs_int32 error, code, bodysize;
	char* body;
	struct HTTPHeader* header;
} HTTPResponse;

typedef struct HTTPRequest {
	Socket sock;
	cs_int32 error;
	struct sockaddr_in addr;
	const char* path;
	HTTPHeader* header;
} HTTPRequest;

API const char* HttpCode_GetReason(cs_int32 code);

API HTTPHeader* HttpRequest_GetHeader(HTTPRequest* req, const char* key);
API void HttpRequest_SetHeaderStr(HTTPRequest* req, const char* key, const char* value);
API const char* HttpRequest_GetHeaderStr(HTTPRequest* req, const char* key);
API void HttpRequest_SetHeaderInt(HTTPRequest* req, const char* key, cs_int32 value);
cs_int32 HttpRequest_GetHeaderInt(HTTPRequest* req, const char* key);
API void HttpRequest_SetHost(HTTPRequest* req, const char* host, cs_uint16 port);
API void HttpRequest_SetPath(HTTPRequest* req, const char* path);
API cs_bool HttpRequest_Read(HTTPRequest* req, Socket sock);
API cs_bool HttpRequest_Perform(HTTPRequest* req, HTTPResponse* resp);
API void HttpRequest_Cleanup(HTTPRequest* req);

API HTTPHeader* HttpResponse_GetHeader(HTTPResponse* resp, const char* key);
API void HttpResponse_SetHeaderStr(HTTPResponse* resp, const char* key, const char* value);
API const char* HttpResponse_GetHeaderStr(HTTPResponse* resp, const char* key);
API void HttpResponse_SetHeaderInt(HTTPResponse* resp, const char* key, cs_int32 value);
API cs_int32 HttpResponse_GetHeaderInt(HTTPResponse* resp, const char* key);
API void HttpResponse_SetHeader(HTTPResponse* resp, const char* key, const char* value);
API void HttpResponse_SetBody(HTTPResponse* resp, char* body, cs_int32 size);
API cs_bool HttpResponse_SendTo(HTTPResponse* resp, Socket sock);
API cs_bool HttpResponse_Read(HTTPResponse* resp, Socket sock);
API void HttpResponse_Cleanup(HTTPResponse* resp);
#endif // HTTP_H
