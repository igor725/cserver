#include "core.h"
#include "log.h"
#include "error.h"
#include "server.h"
#include "client.h"
#include "packets.h"
#include "config.h"
#include "console.h"
#include "command.h"
#include "platform.h"

void Server_Accept() {
	struct sockaddr_in caddr;
	int caddrsz = sizeof caddr;

	SOCKET fd = accept(server, (struct sockaddr*)&caddr, &caddrsz);
	if(fd != INVALID_SOCKET) {
	 	CLIENT* tmp = calloc(1, sizeof(struct client));

		tmp->sock = fd;
		tmp->bufpos = 0;
		tmp->rdbuf = calloc(131, 1);
		tmp->wrbuf = calloc(2048, 1);

		int id = Client_FindFreeID();
		if(id >= 0) {
			tmp->id = id;
			tmp->status = CLIENT_OK;
			tmp->thread = Thread_Create(&Client_ThreadProc, tmp);
			clients[id] = tmp;
		} else {
			Client_Kick(tmp, "Server is full");
		}
	}
}

int Server_AcceptThread(void* lpParam) {
	while(1)Server_Accept();
	return 0;
}

bool Server_Bind(char* ip, short port) {
	if((server = Socket_Bind(ip, port)) == INVALID_SOCKET)
		return false;

	Client_Init();
	acceptThread = Thread_Create(&Server_AcceptThread, NULL);
	Log_Info("%s %s started on %s:%d", SOFTWARE_NAME, SOFTWARE_VERSION, ip, port);
	return true;
}

bool Server_InitialWork() {
	if(!Socket_Init())
		return false;
	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();
	Command_RegisterDefault();

	worlds[0] = World_Create("TestWorld");
	if(!World_Load(worlds[0])){
		Log_FormattedError();
		World_SetDimensions(worlds[0], 128, 128, 128);
		World_AllocBlockArray(worlds[0]);
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
			if(!World_Save(world))
				Log_FormattedError();
			World_Destroy(world);
		}
	}

	if(acceptThread)
		Thread_Close(acceptThread);
	Console_Close();
	Socket_Close(server);
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
