#include "core.h"
#include "world.h"
#include "server.h"
#include "client.h"
#include "packets.h"
#include "config.h"
#include "console.h"
#include "command.h"
#include "generators.h"
#include "event.h"
#include "cplugin.h"

void Server_Accept() {
	struct sockaddr_in caddr;
	socklen_t caddrsz = sizeof caddr;

	SOCKET fd = accept(Server_Socket, (struct sockaddr*)&caddr, &caddrsz);

	if(fd != INVALID_SOCKET) {
		if(!Server_Active) {
			Socket_Close(fd);
			return;
		}
	 	CLIENT* tmp = (CLIENT*)Memory_Alloc(1, sizeof(CLIENT));

		tmp->sock = fd;
		tmp->bufpos = 0;
		tmp->mutex = Mutex_Create();
		tmp->addr = ntohl(caddr.sin_addr.s_addr);
		tmp->rdbuf = (char*)Memory_Alloc(131, 1);
		tmp->wrbuf = (char*)Memory_Alloc(2048, 1);

		ClientID id = Client_FindFreeID();
		if(id >= 0) {
			tmp->id = id;
			tmp->status = CLIENT_OK;
			tmp->thread = Thread_Create(Client_ThreadProc, tmp);
			if(!Thread_IsValid(tmp->thread)) {
				Log_FormattedError();
				Client_Kick(tmp, "Can't create packet handling thread");
				return;
			}

			Clients_List[id] = tmp;
		} else {
			Client_Kick(tmp, "Server is full");
		}
	}
}

TRET Server_ThreadProc(void* lpParam) {
	Thread_SetName("AcceptThread");
	while(Server_Active)Server_Accept();
	return 0;
}

bool Server_Bind(const char* ip, ushort port) {
	if((Server_Socket = Socket_Bind(ip, port)) == INVALID_SOCKET)
		return false;

	Client_Init();
	Log_Info("%s %s started on %s:%d", SOFTWARE_NAME, SOFTWARE_VERSION, ip, port);
	return true;
}

static void evt_onconnect(void* param) {
	CLIENT* cl = (CLIENT*)param;
	Log_Info("Player %s connected", cl->playerData->name);
}

static void evt_ondisconnect(void* param) {
	CLIENT* cl = (CLIENT*)param;
	Log_Info("Player %s disconnected", cl->playerData->name);
}

bool Server_InitialWork() {
	if(!Socket_Init())
		return false;

	Log_Info("Loading " MAINCFG);
	Server_Config = Config_Create(MAINCFG);
	Config_SetStr(Server_Config, "ip", "0.0.0.0");
	Config_SetInt(Server_Config, "port", 25565);
	Config_SetStr(Server_Config, "name", DEFAULT_NAME);
	Config_SetStr(Server_Config, "motd", DEFAULT_MOTD);
	Config_SetInt(Server_Config, "loglevel", 3);
	Config_SetBool(Server_Config, "alwayslocalop", false);
	Config_Load(Server_Config);

	Log_SetLevel(Config_GetInt(Server_Config, "loglevel"));
	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();
	Command_RegisterDefault();
	Event_RegisterVoid(EVT_ONHANDSHAKEDONE, evt_onconnect);
	Event_RegisterVoid(EVT_ONDISCONNECT, evt_ondisconnect);

	Directory_Ensure("worlds");
	int wIndex = -1;
	dirIter wIter = {0};
	if(Iter_Init(&wIter, "worlds", "cws")) {
		do {
			if(wIter.isDir || !wIter.cfile) continue;
			WORLD* tmp = World_Create(wIter.cfile);
			if(!World_Load(tmp)) {
				Log_FormattedError();
				World_Destroy(tmp);
			} else
				Worlds_List[++wIndex] = tmp;
		} while(Iter_Next(&wIter) && wIndex < MAX_WORLDS);
	} else
		Log_FormattedError();
	Iter_Close(&wIter);

	if(wIndex < 0) {
		WORLD* tmp = World_Create("world.cws");
		World_SetDimensions(tmp, 256, 256, 256);
		World_AllocBlockArray(tmp);
		Generator_Flat(tmp);
		Worlds_List[0] = tmp;
	}

	Log_Info("Loading C plugins");
	CPlugin_Start();

	Console_StartListen();
	Event_Call(EVT_POSTSTART, NULL);
	return Server_Bind(Config_GetStr(Server_Config, "ip"), (ushort)Config_GetInt(Server_Config, "port"));
}

void Server_DoStep() {
	Event_Call(EVT_ONTICK, NULL);
	for(int i = 0; i < MAX_CLIENTS; i++) {
		CLIENT* client = Clients_List[i];
		if(client)
			Client_Tick(client);
	}
}

void Server_Stop() {
	Event_Call(EVT_ONSTOP, NULL);
	Log_Info("Saving worlds");
	for(int i = 0; i < max(MAX_WORLDS, MAX_CLIENTS); i++) {
		CLIENT* client = Clients_List[i];
		WORLD* world = Worlds_List[i];

		if(i < MAX_CLIENTS && client)
			Client_Kick(client, "Server stopped");

		if(i < MAX_WORLDS && world) {
			if(!World_Save(world))
				Log_FormattedError();
			World_Destroy(world);
		}
	}

	Console_Close();
	if(Server_AcceptThread)
		Thread_Close(Server_AcceptThread);

	Log_Info("Unloading plugins");
	CPlugin_Stop();

	Socket_Close(Server_Socket);
	Log_Info("Saving server.cfg");
	Config_Save(Server_Config);
}

int main(int argc, char** argv) {
	if(!(Server_Active = Server_InitialWork())) {
		Log_FormattedError();
	} else {
		Server_AcceptThread = Thread_Create(Server_ThreadProc, NULL);
		if(!Thread_IsValid(Server_AcceptThread)) {
			Log_Error("Can't create accept thread");
			Server_Active = false;
		}
	}

	while(Server_Active) {
		Server_DoStep();
		Sleep(10);
	}

	Log_Info("Main loop done");
	Server_Stop();
	return 0;
}
