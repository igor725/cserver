#include <core.h>

#include "http.h"

SOCKET httpServer;

static char* ReadSockUntil(WEBCLIENT* wcl, size_t len, char sym) {
	char* tmp = wcl->buffer;

	do {
		int ret = recv(wcl->sock, tmp, 1, 0);
		if(ret > 0)
			len -= ret;
		else
			break;
	}
	while(len > 0 && *tmp != sym && (*tmp == '\r' ? tmp : tmp++));

	*tmp = '\0';
	return wcl->buffer;
}

static void ReadHeader(WEBCLIENT* wcl, const char* value) {
	const char* key = wcl->buffer;

	if(String_CaselessCompare(key, "")) {
		;
	}
}

static void Handle_GetRequest(WEBCLIENT* wcl, char* buffer) {
	wcl->request = String_AllocCopy(ReadSockUntil(wcl, 512, ' '));
	char* httpver = ReadSockUntil(wcl, 1024, '\n');

	while(1) {
		char* value = ReadSockUntil(wcl, 1024, '\n');
		if(!String_Length(value)) break;

		while(*value != ':') ++value;
		*value = '\0'; value += 2;
		ReadHeader(wcl, value);
	}
}

static TRET ClientThreadProc(TARG param) {
	WEBCLIENT* wcl = (WEBCLIENT*)Memory_Alloc(1, sizeof(WEBCLIENT));
	wcl->sock = (SOCKET)param;
	recv(wcl->sock, wcl->buffer, 4, 0);

	if(String_CaselessCompare(wcl->buffer, "get ")) {
		Handle_GetRequest(wcl, wcl->buffer);
	}
	Socket_Close(wcl->sock);

	if(wcl->request)
		Memory_Free((void*)wcl->request);
	Memory_Free(wcl);

	return 0;
}

static TRET AcceptThreadProc(TARG param) {
	struct sockaddr_in caddr;
	socklen_t caddrsz = sizeof caddr;

	while(1) {
		SOCKET client = accept(httpServer, (struct sockaddr*)&caddr, &caddrsz);

		if(client != INVALID_SOCKET) {
			Thread_Create(ClientThreadProc, (TARG)client);
		} else {
			break;
		}
	}

	Socket_Close(httpServer);
	return 0;
}

bool Http_StartServer(const char* ip, ushort port) {
	httpServer = Socket_Bind(ip, port);
	if(httpServer != INVALID_SOCKET) {
		Thread_Create(AcceptThreadProc, NULL);
		return true;
	}
	return false;
}
