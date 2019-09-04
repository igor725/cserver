#include "core.h"
#include "world.h"
#include "server.h"
#include "client.h"
#include "packets.h"
#include "config.h"
#include "console.h"
#include "command.h"
#include "generators.h"
#ifdef LUA_ENABLED
#include "luaplugin.h"
#endif

void Server_Accept() {
	struct sockaddr_in caddr;
	socklen_t caddrsz = sizeof caddr;

	SOCKET fd = accept(server, (struct sockaddr*)&caddr, &caddrsz);

	if(fd != INVALID_SOCKET) {
		if(!serverActive) {
			Socket_Close(fd);
			return;
		}
	 	CLIENT* tmp = (CLIENT*)Memory_Alloc(1, sizeof(CLIENT));

		tmp->sock = fd;
		tmp->bufpos = 0;
		tmp->mutex = Mutex_Create();
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

THRET Server_ThreadProc(void* lpParam) {
	Thread_SetName("AcceptThread");
	while(serverActive)Server_Accept();
	return 0;
}

bool Server_Bind(const char* ip, ushort port) {
	if((server = Socket_Bind(ip, port)) == INVALID_SOCKET)
		return false;

	Client_Init();
	Log_Info("%s %s started on %s:%d", SOFTWARE_NAME, SOFTWARE_VERSION, ip, port);
	return true;
}

bool Server_InitialWork() {
	if(!Socket_Init())
		return false;

	Log_Info("Loading " MAINCFG);
	mainCfg = Config_Create(MAINCFG);
	if(!Config_Load(mainCfg)) {
		Config_EmptyStore(mainCfg);
		Config_SetStr(mainCfg, "ip", "0.0.0.0");
		Config_SetInt(mainCfg, "port", 25565);
		Config_SetStr(mainCfg, "name", DEFAULT_NAME);
		Config_SetStr(mainCfg, "motd", DEFAULT_MOTD);
	}

	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();
	Command_RegisterDefault();

	int wIndex = -1;
	dirIter wIter = {0};
	if(Iter_Init(&wIter, ".", "cws")) {
		do {
			if(wIter.isDir || !wIter.cfile) continue;
			WORLD* tmp = World_Create(wIter.cfile);
			if(!World_Load(tmp)) {
				Log_FormattedError();
				World_Destroy(tmp);
			} else {
				Log_Info("World \"%s\" loaded", wIter.cfile);
				worlds[++wIndex] = tmp;
			}
		} while(Iter_Next(&wIter) && wIndex < MAX_WORLDS);
	} else
		Log_FormattedError();
	Iter_Close(&wIter);

	if(wIndex < 0) {
		WORLD* tmp = World_Create("world.cws");
		World_SetDimensions(tmp, 128, 128, 128);
		World_AllocBlockArray(tmp);
		Generator_Flat(tmp);
		worlds[0] = tmp;
	}

#ifdef LUA_ENABLED
	Log_Info("Starting LuaVM");
	LuaPlugin_Start();
#endif

	Console_StartListen();
	return Server_Bind(Config_GetStr(mainCfg, "ip"), (ushort)Config_GetInt(mainCfg, "port"));
}

void Server_DoStep() {
#ifdef LUA_ENABLED
	LuaPlugin_Tick();
#endif
	for(int i = 0; i < MAX_CLIENTS; i++) {
		CLIENT* client = clients[i];
		if(client)
			Client_Tick(client);
	}
}

void Server_Stop() {
	for(int i = 0; i < max(MAX_WORLDS, MAX_CLIENTS); i++) {
		CLIENT* client = clients[i];
		WORLD* world = worlds[i];

		if(i < MAX_CLIENTS && client)
			Client_Kick(client, "Server stopped");

		if(i < MAX_WORLDS && world) {
			if(!World_Save(world))
				Log_FormattedError();
			World_Destroy(world);
		}
	}

	Console_Close();
	if(acceptThread)
		Thread_Close(acceptThread);
#ifdef LUA_ENABLED
	Log_Info("Destroying LuaVM");
	LuaPlugin_Stop();
#endif
	Socket_Close(server);
	Config_Save(mainCfg);
}

int main(int argc, char** argv) {
	if(!(serverActive = Server_InitialWork())) {
		Log_FormattedError();
	} else {
		acceptThread = Thread_Create((TFUNC)&Server_ThreadProc, NULL);
		if(!Thread_IsValid(acceptThread)) {
			Log_Error("acceptThread == NULL");
			serverActive = false;
		}
	}

	while(serverActive) {
		Server_DoStep();
		Sleep(10);
	}

	Log_Info("Server stopped");
	Server_Stop();
}
