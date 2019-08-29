#include "core.h"
#include "log.h"
#include "server.h"
#include "client.h"
#include "packets.h"
#include "config.h"
#include "console.h"
#include "command.h"

void Server_Accept() {
	struct sockaddr_in caddr;
	int caddrsz = sizeof caddr;

	SOCKET fd = accept(server, (struct sockaddr*)&caddr, &caddrsz);
	if(fd != INVALID_SOCKET) {
	 	CLIENT* tmp = (CLIENT*)Memory_Alloc(1, sizeof(CLIENT));

		tmp->sock = fd;
		tmp->bufpos = 0;
		tmp->rdbuf = (char*)Memory_Alloc(131, 1);
		tmp->wrbuf = (char*)Memory_Alloc(2048, 1);

		ClientID id = Client_FindFreeID();
		if(id >= 0) {
			tmp->id = id;
			tmp->status = CLIENT_OK;
			tmp->thread = Thread_Create((TFUNC)&Client_ThreadProc, tmp);
			if(!Thread_IsValid(tmp->thread)) {
				Log_FormattedError();
				Client_Kick(tmp, "client->thread == NULL");
				return;
			}

			clients[id] = tmp;
		} else {
			Client_Kick(tmp, "Server is full");
		}
	}
}

THRET Server_AcceptThread(void* lpParam) {
	Thread_SetName("AcceptThread");
	while(1)Server_Accept();
	return 0;
}

bool Server_Bind(char* ip, ushort port) {
	if((server = Socket_Bind(ip, port)) == INVALID_SOCKET)
		return false;

	Client_Init();
	acceptThread = Thread_Create((TFUNC)&Server_AcceptThread, NULL);
	if(Thread_IsValid(acceptThread))
		Log_Info("%s %s started on %s:%d", SOFTWARE_NAME, SOFTWARE_VERSION, ip, port);
	else {
		Log_Error("acceptThread == NULL");
		return false;
	}
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
	Log_Info("Loading server.cfg");
	if(Config_Load("./server.cfg"))
		return Server_Bind(Config_GetStr("ip"), (ushort)Config_GetInt("port"));
	else
		return false;
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
