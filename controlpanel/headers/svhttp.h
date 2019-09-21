#ifndef CP_HTTP_H
#define CP_HTTP_H
#include "websocket.h"

#define HTTP_SERVER_NAME "CPL Server v1"
#define CPL_WSPROTO "CPLwsprotov1"
#define HTTP_BUFFER_LEN 1024

typedef struct webHeader {
	const char* key;
	const char* value;
} WEBHEADER;

typedef struct webClient {
	SOCKET sock;
	char buffer[1024];
	const char* respBody;
	size_t respLength;
	int respCode;
	const char* request;
	bool wsUpgrade;
	int wsVersion;
	const char* wsKey;
	WSFRAME* wsFrame;
} WEBCLIENT;

bool Http_StartServer(const char* ip, ushort port);
bool Http_CloseServer();
#endif