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
	Client_InitListen();
}

void DoServerStep() {
	//TODO: Ыа?
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

	//setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 1);
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
