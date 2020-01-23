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
	cs_str key;
	cs_int32 type;
	union {
		cs_str vchar;
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
	cs_str path;
	HTTPHeader* header;
} HTTPRequest;

API cs_str HttpCode_GetReason(cs_int32 code);

API HTTPHeader* HttpRequest_GetHeader(HTTPRequest* req, cs_str key);
API void HttpRequest_SetHeaderStr(HTTPRequest* req, cs_str key, cs_str value);
API cs_str HttpRequest_GetHeaderStr(HTTPRequest* req, cs_str key);
API void HttpRequest_SetHeaderInt(HTTPRequest* req, cs_str key, cs_int32 value);
cs_int32 HttpRequest_GetHeaderInt(HTTPRequest* req, cs_str key);
API cs_bool HttpRequest_SetHost(HTTPRequest* req, cs_str host, cs_uint16 port, cs_bool addwww);
API void HttpRequest_SetPath(HTTPRequest* req, cs_str path);
API cs_bool HttpRequest_Read(HTTPRequest* req, Socket sock);
API cs_bool HttpRequest_Perform(HTTPRequest* req, HTTPResponse* resp);
API void HttpRequest_Cleanup(HTTPRequest* req);

API HTTPHeader* HttpResponse_GetHeader(HTTPResponse* resp, cs_str key);
API void HttpResponse_SetHeaderStr(HTTPResponse* resp, cs_str key, cs_str value);
API cs_str HttpResponse_GetHeaderStr(HTTPResponse* resp, cs_str key);
API void HttpResponse_SetHeaderInt(HTTPResponse* resp, cs_str key, cs_int32 value);
API cs_int32 HttpResponse_GetHeaderInt(HTTPResponse* resp, cs_str key);
API void HttpResponse_SetHeader(HTTPResponse* resp, cs_str key, cs_str value);
API void HttpResponse_SetBody(HTTPResponse* resp, char* body, cs_int32 size);
API cs_bool HttpResponse_SendTo(HTTPResponse* resp, Socket sock);
API cs_bool HttpResponse_Read(HTTPResponse* resp, Socket sock);
API void HttpResponse_Cleanup(HTTPResponse* resp);
#endif // HTTP_H
