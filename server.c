#include "server.h"
#include "client.h"
#include "packets.h"
#include "config.h"
#include "console.h"
#include "command.h"

bool Server_Bind(char* ip, short port) {
	WSADATA ws;
	if(WSAStartup(MAKEWORD(1, 1), &ws) == SOCKET_ERROR)
		Error_SetCode(ET_WIN, WSAGetLastError(), "WSAStartup");

	if(INVALID_SOCKET == (server = socket(AF_INET, SOCK_STREAM, 0)))
		Error_SetCode(ET_WIN, WSAGetLastError(), "socket");

	struct sockaddr_in ssa;
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(port);
	ssa.sin_addr.s_addr = inet_addr(ip);

	if(bind(server, (const struct sockaddr*)&ssa, sizeof ssa) == -1) {
		Error_SetCode(ET_WIN, WSAGetLastError(), "bind");
		return false;
	}

	if(listen(server, SOMAXCONN) == -1) {
		Error_SetCode(ET_WIN, WSAGetLastError(), "listen");
		return false;
	}


	Client_Init();
	Log_Info("%s %s started on %s:%d", SOFTWARE_NAME, SOFTWARE_VERSION, ip, port);
	return true;
}

bool Server_InitialWork() {
	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();
	Command_RegisterDefault();

	worlds[0] = World_Create("TestWorld", 128, 128, 128);
	if(!World_Load(worlds[0])){
		Log_FormattedError();
		World_GenerateFlat(worlds[0]);
	}

	Console_StartListen();
	if(Config_Load("server.cfg"))
		return Server_Bind(Config_GetStr("ip"), Config_GetInt("port"));
	else
		return false;

	return true;
}

void Server_DoStep() {
	for(int i = 0; i < 128; i++) {
		CLIENT* client = clients[i];
		if(client)
			Client_Tick(client);
	}
}

void Server_Stop() {
	for(int i = 0; i < 128; i++) {
		CLIENT* client = clients[i];
		WORLD* world = worlds[i];

		if(client)
			Client_Kick(client, "Server stopped");

		if(world) {
			// World_Save(world);
			World_Destroy(world);
		}
	}

	Console_StopListen();
}

int main(int argc, char** argv) {
	if(!(serverActive = Server_InitialWork()))
		Log_FormattedError();

	while(serverActive) {
		Server_DoStep();
		Sleep(10);
	}

	Log_Info("Server stopped");
	Server_Stop();
}
