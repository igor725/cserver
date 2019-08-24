#include <winsock2.h>
#include "core.h"
#include "log.h"
#include "world.h"
#include "client.h"
#include "server.h"
#include "packets.h"

void Server_InitialWork() {
	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();
	Client_InitListen();
	World = World_Create("TestWorld", 128, 128, 128);
	if(!World_Load(World)) {
		Log_WinErr("World_Load()");
	}
}

void Server_DoStep() {
	for(int i = 0; i < 128; i++) {
		CLIENT* self = clients[i];
		if(self == NULL)
			continue;
		if(self->state == CLIENT_AFTERCLOSE) {
			CloseHandle(self->thread);
			Client_Destroy(self);
			clients[i] = NULL;
		}
	}
}

int main(int argc, char** argv) {
	WSADATA ws;
	if(WSAStartup(MAKEWORD(1, 1), &ws) == SOCKET_ERROR) {
		Log_WSAErr("WSAStartup()");
	}

	if(INVALID_SOCKET == (server = socket(AF_INET, SOCK_STREAM, 0)))
		Log_WSAErr("socket()");

	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(25565);
	ssa.sin_addr.s_addr = inet_addr("0.0.0.0");

	//setsockopt(server, SOL_SOCKET, SO_REUSEADDR, 1);
	if(bind(server, (const struct sockaddr*)&ssa, sizeof ssa) == -1)
		Log_WSAErr("bind()");

	if(listen(server, SOMAXCONN) == -1)
		Log_WSAErr("listen()");

	Log_Info("%s %s started on 0.0.0.0:25565", SOFTWARE_NAME, SOFTWARE_VERSION);
	Server_InitialWork();
	while(1) {
		Server_DoStep();
		Sleep(10);
	}
}
