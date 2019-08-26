#include "server.h"
#include "client.h"
#include "packets.h"
#include "config.h"
#include "console.h"
#include "command.h"

void Server_Bind(char* ip, short port) {
	WSADATA ws;
	if(WSAStartup(MAKEWORD(1, 1), &ws) == SOCKET_ERROR)
		Log_WSAErr("WSAStartup()");

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


	Client_Listen();
	Log_Info("%s %s started on %s:%d", SOFTWARE_NAME, SOFTWARE_VERSION, ip, port);
}

bool Server_InitialWork() {
	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();
	Command_RegisterDefault();

	worlds[0] = World_Create("TestWorld", 128, 128, 128);
	if(!World_Load(worlds[0])) {
		Log_WinErr("World_Load()");
	}

	if(Config_Load("test.cfg"))
		Server_Bind(Config_GetStr("IP"), Config_GetInt("PORT"));
	else
		return false;

	Console_StartListen();

	return true;
}

void Server_DoStep() {
	for(int i = 0; i < 128; i++) {
		CLIENT* client = clients[i];
		if(client == NULL) continue;

		if(client->status == CLIENT_AFTERCLOSE) {
			Client_Destroy(client);
			clients[i] = NULL;
		}

		if(client->playerData == NULL) continue;
		if(client->playerData->state == STATE_INGAME) {
			if(client->playerData->mapThread != NULL) {
				CloseHandle(client->playerData->mapThread);
				client->playerData->mapThread = NULL;
			}
		}
	}
}

void Server_Stop() {
	for(int i = 0; i < 128; i++) {
		CLIENT* client = clients[i];
		WORLD* world = worlds[i];

		if(client)
			Packet_WriteKick(client, "Server stopped");

		if(world)
			World_Save(world);
	}

	Console_StopListen();
}

int main(int argc, char** argv) {
	serverActive = Server_InitialWork();
	while(serverActive) {
		Server_DoStep();
		Sleep(10);
	}
	Log_Info("Server stopped");
	Server_Stop();
}
