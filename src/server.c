#include "core.h"
#include "str.h"
#include "log.h"
#include "error.h"
#include "server.h"
#include "client.h"
#include "protocol.h"
#include "config.h"
#include "generators.h"
#include "heartbeat.h"
#include "platform.h"
#include "event.h"
#include "plugin.h"
#include "lang.h"
#include "timer.h"
#include "consoleio.h"

THREAD_FUNC(ClientInitThread) {
	Client *tmp = (Client *)param;
	cs_int8 maxConnPerIP = Config_GetInt8ByKey(Server_Config, CFG_CONN_KEY),
	sameAddrCount = 1;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(other && other->addr == tmp->addr)
			++sameAddrCount;
		else continue;

		if(sameAddrCount > maxConnPerIP) {
			Client_Kick(tmp, Lang_Get(Lang_KickGrp, 10));
			Client_Free(tmp);
			return 0;
		}
	}

	cs_byte attempt = 0;
	while(attempt < 5) {
		if(Socket_Receive(tmp->sock, tmp->rdbuf, 5, MSG_PEEK) == 5) {
			if(String_CaselessCompare(tmp->rdbuf, "GET /")) {
				WebSock *wscl = Memory_Alloc(1, sizeof(WebSock));
				wscl->proto = "ClassiCube";
				wscl->recvbuf = tmp->rdbuf;
				wscl->sock = tmp->sock;
				tmp->websock = wscl;
				if(WebSock_DoHandshake(wscl))
					goto client_ok;
				else break;
			} else goto client_ok;
		}
		Thread_Sleep(100);
		attempt++;
	}

	Client_Kick(tmp, Lang_Get(Lang_KickGrp, 7));
	Client_Free(tmp);
	return 0;

	client_ok:
	if(!Client_Add(tmp)) {
		Client_Kick(tmp, Lang_Get(Lang_KickGrp, 1));
		Client_Free(tmp);
	}

	return 0;
}

THREAD_FUNC(AcceptThread) {
	(void)param;
	struct sockaddr_in caddr;

	while(Server_Active) {
		Socket fd = Socket_Accept(Server_Socket, &caddr);

		if(fd != INVALID_SOCKET) {
			if(!Server_Active) {
				Socket_Close(fd);
				break;
			}

			Client *tmp = Client_New(fd, ntohl(caddr.sin_addr.s_addr));
			if(tmp)
				Thread_Create(ClientInitThread, tmp, true);
			else
				Socket_Close(fd);
		}
	}

	return 0;
}

static void Bind(cs_str ip, cs_uint16 port) {
	Server_Socket = Socket_New();
	if(!Server_Socket) {
		Error_PrintSys(true);
	}
	struct sockaddr_in ssa;
	switch (Socket_SetAddr(&ssa, ip, port)) {
		case 0:
			ERROR_PRINT(ET_SERVER, EC_INVALIDIP, true);
			break;
		case -1:
			Error_PrintSys(true);
			break;
	}

	Client_Init();
	Log_Info(Lang_Get(Lang_ConGrp, 0), ip, port);
	if(!Socket_Bind(Server_Socket, &ssa)) {
		Error_PrintSys(true);
	}
}

cs_bool Server_Init(void) {
	if(!Socket_Init() || !Lang_Init() || !Generators_Init()) return false;

	CStore *cfg = Config_NewStore(MAINCFG);
	CEntry *ent;

	Server_StartTime = Time_GetMSec();
	Server_Config = cfg;

	ent = Config_NewEntry(cfg, CFG_SERVERIP_KEY, CFG_TSTR);
	Config_SetComment(ent, "Bind server to specified IP address. \"0.0.0.0\" - means \"all available network adapters\".");
	Config_SetDefaultStr(ent, "0.0.0.0");

	ent = Config_NewEntry(cfg, CFG_SERVERPORT_KEY, CFG_TINT16);
	Config_SetComment(ent, "Use specified port to accept clients. [1-65535]");
	Config_SetLimit(ent, 1, 65535);
	Config_SetDefaultInt16(ent, 25565);

	ent = Config_NewEntry(cfg, CFG_SERVERNAME_KEY, CFG_TSTR);
	Config_SetComment(ent, "Server name and MOTD will be shown to the player during map loading.");
	Config_SetDefaultStr(ent, "Server name");

	ent = Config_NewEntry(cfg, CFG_SERVERMOTD_KEY, CFG_TSTR);
	Config_SetDefaultStr(ent, "Server MOTD");

	ent = Config_NewEntry(cfg, CFG_LOGLEVEL_KEY, CFG_TSTR);
	Config_SetComment(ent, "I - Info, C - Chat, W - Warnings, D - Debug.");
	Config_SetDefaultStr(ent, "ICWD");

	ent = Config_NewEntry(cfg, CFG_LOCALOP_KEY, CFG_TBOOL);
	Config_SetComment(ent, "Any player with ip address \"127.0.0.1\" will automatically become an operator.");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_MAXPLAYERS_KEY, CFG_TINT8);
	Config_SetComment(ent, "Max players on server. [1-127]");
	Config_SetLimit(ent, 1, 127);
	Config_SetDefaultInt8(ent, 10);

	ent = Config_NewEntry(cfg, CFG_CONN_KEY, CFG_TINT8);
	Config_SetComment(ent, "Max connections per one IP. [1-5]");
	Config_SetLimit(ent, 1, 5);
	Config_SetDefaultInt8(ent, 5);

	ent = Config_NewEntry(cfg, CFG_HEARTBEAT_KEY, CFG_TBOOL);
	Config_SetComment(ent, "Enable ClassiCube heartbeat.");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_HEARTBEATDELAY_KEY, CFG_TINT32);
	Config_SetComment(ent, "Heartbeat request delay. [1-60]");
	Config_SetLimit(ent, 1, 60);
	Config_SetDefaultInt32(ent, 10);

	ent = Config_NewEntry(cfg, CFG_HEARTBEAT_PUBLIC_KEY, CFG_TBOOL);
	Config_SetComment(ent, "Show server in the ClassiCube server list.");
	Config_SetDefaultBool(ent, false);

	cfg->modified = true;
	if(!Config_Load(cfg)) {
		Config_PrintError(cfg);
		return false;
	}
	Log_SetLevelStr(Config_GetStrByKey(cfg, CFG_LOGLEVEL_KEY));

	Packet_RegisterDefault();
	Plugin_LoadAll();

	Directory_Ensure("worlds");
	WorldID wIndex = 0;
	DirIter wIter;
	if(Iter_Init(&wIter, "worlds", "cws")) {
		do {
			if(wIter.isDir || !wIter.cfile) continue;
			World *tmp = World_Create(wIter.cfile);
			tmp->id = wIndex++;
			if(!World_Load(tmp) || !World_Add(tmp))
				World_Free(tmp);
		} while(Iter_Next(&wIter) && wIndex < MAX_WORLDS);
	}
	Iter_Close(&wIter);

	if(wIndex < 1) {
		World *tmp = World_Create("world.cws");
		SVec defdims = {256, 256, 256};
		World_SetDimensions(tmp, &defdims);
		World_AllocBlockArray(tmp);
		if(!Generators_Use(tmp, "flat", NULL))
			Log_Error("Oh! Error happened in the world generator.");
		Worlds_List[0] = tmp;
	}

	if(Config_GetBoolByKey(cfg, CFG_HEARTBEAT_KEY))
		Heartbeat_Start(Config_GetInt32ByKey(cfg, CFG_HEARTBEATDELAY_KEY));

	Server_Active = true;
	cs_str ip = Config_GetStrByKey(cfg, CFG_SERVERIP_KEY);
	cs_uint16 port = Config_GetInt16ByKey(cfg, CFG_SERVERPORT_KEY);
	Thread_Create(AcceptThread, NULL, true);
	Bind(ip, port);
	Event_Call(EVT_POSTSTART, NULL);
	ConsoleIO_Init();
	return true;
}

void Server_DoStep(cs_int32 delta) {
	Event_Call(EVT_ONTICK, &delta);
	Timer_Update(delta);
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client) Client_Tick(client, delta);
	}
}

void Server_StartLoop(void) {
	if(!Server_Active) return;
	cs_uint64 last, curr = Time_GetMSec();
	cs_int32 delta;

	while(Server_Active) {
		last = curr;
		curr = Time_GetMSec();
		delta = (cs_int32)(curr - last);
		if(delta < 0) {
			Log_Warn(Lang_Get(Lang_ConGrp, 2));
			delta = 0;
		}
		if(delta > 500) {
			Log_Warn(Lang_Get(Lang_ConGrp, 1), delta);
			delta = 500;
		}
		Server_DoStep(delta);
		Thread_Sleep(10);
	}
}

void Server_Stop(void) {
	Event_Call(EVT_ONSTOP, NULL);
	Log_Info(Lang_Get(Lang_ConGrp, 4));
	Clients_KickAll(Lang_Get(Lang_KickGrp, 5));
	Log_Info(Lang_Get(Lang_ConGrp, 5));
	Worlds_SaveAll(true, true);
	Socket_Close(Server_Socket);
	Config_Save(Server_Config);
	Config_DestroyStore(Server_Config);
	Plugin_UnloadAll();
}
