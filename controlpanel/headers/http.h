#ifndef CP_HTTP_H
#define CP_HTTP_H
#include "websocket.h"

typedef struct webClient {
	SOCKET sock;
	char buffer[1024];
	const char* request;
	bool websocketUpgrade;
	WSFRAME* wsFrame;
} WEBCLIENT;
#endif
