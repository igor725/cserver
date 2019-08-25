#include <winsock2.h>
#include "core.h"
#include "log.h"
#include "world.h"
#include "client.h"
#include "server.h"
#include "packets.h"
#include "config.h"

void Server_Bind(char* ip, short port) {
	WSADATA ws;
	if(WSAStartup(MAKEWORD(1, 1), &ws) == SOCKET_ERROR) {
		Log_WSAErr("WSAStartup()");
	}

	if(INVALID_SOCKET == (server = socket(AF_INET, SOCK_STREAM, 0)))
		Log_WSAErr("socket()");

	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(port);
	ssa.sin_addr.s_addr = inet_addr(ip);

	if(bind(server, (const struct sockaddr*)&ssa, sizeof ssa) == -1)
		Log_WSAErr("bind()");

	if(listen(server, SOMAXCONN) == -1)
		Log_WSAErr("listen()");

	Client_InitListen();
	Log_Info("%s %s started on %s:%d", SOFTWARE_NAME, SOFTWARE_VERSION, ip, port);
}

bool Server_InitialWork() {
	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();

	worlds[0] = World_Create("TestWorld", 128, 128, 128);
	if(!World_Load(worlds[0])) {
		Log_WinErr("World_Load()");
	}

	if(Config_Load("test.cfg"))
		Server_Bind(Config_GetStr("IP"), Config_GetInt("PORT"));
	else
		return false;

	return true;
}

void Server_DoStep() {
	for(int i = 0; i < 128; i++) {
		CLIENT* self = clients[i];
		if(self == NULL) continue;

		if(self->status == CLIENT_AFTERCLOSE) {
			Client_Destroy(self);
			clients[i] = NULL;
		}
	}
}

void Server_Stop() {
	for(int i = 0; i < 128; i++) {
		CLIENT* self = clients[i];
		WORLD* world = worlds[i];

		if(self != NULL)
			Packet_WriteKick(self, "Server stopped");

		if(world != NULL)
			World_Save(world);
	}
}

int main(int argc, char** argv) {
	serverActive = Server_InitialWork();
	while(serverActive) {
		Server_DoStep();
		Sleep(10);
	}
	Server_Stop();
}
