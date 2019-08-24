#include <winsock2.h>
#include <stdio.h>
#include "core.h"
#include "client.h"
#include "server.h"
#include "packets.h"

void WSAErr(const char* func) {
	printf("%s error: %d", func, WSAGetLastError());
}

void InitialWork() {
	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();
	Client_InitListen();
}

void DoServerStep() {
	for(int i = 0; i < 128; i++) {
		CLIENT* cl = clients[i];
		if(cl == NULL)
			continue;
		if(cl->state == CLIENT_AFTERCLOSE) {
			CloseHandle(cl->thread);
			clients[i] = NULL;
		}
	}
}

int main(int argc, char** argv) {
	WSADATA ws;
	if(WSAStartup(MAKEWORD(1, 1), &ws) == SOCKET_ERROR) {
		WSAErr("WSAStartup()");
	}

	if(INVALID_SOCKET == (server = socket(AF_INET, SOCK_STREAM, 0)))
		WSAErr("socket()");

	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(25565);
	ssa.sin_addr.s_addr = inet_addr("0.0.0.0");

	//setsockopt(server, SOL_SOCKET, SO_REUSEADDR, 1);
	if(bind(server, (const struct sockaddr*)&ssa, sizeof ssa) == -1)
		WSAErr("bind()");

	if(listen(server, SOMAXCONN) == -1)
		WSAErr("listen()");

	printf("%s %s started on 0.0.0.0:25565\n", SOFTWARE_NAME, SOFTWARE_VERSION);
	InitialWork();
	while(1) {
		DoServerStep();
		Sleep(10);
	}
}
