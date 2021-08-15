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
#include <zlib.h>

CStore *Server_Config = NULL;
cs_bool Server_Active = false, Server_Ready = false;
cs_uint64 Server_StartTime = 0, Server_LatestBadTick = 0;
Socket Server_Socket = 0;

static cs_bool AddClient(Client *client) {
	cs_int8 maxplayers = Config_GetInt8ByKey(Server_Config, CFG_MAXPLAYERS_KEY);
	for(ClientID i = 0; i < min(maxplayers, MAX_CLIENTS); i++) {
		if(!Clients_List[i]) {
			client->id = i;
			Clients_List[i] = client;
			return true;
		}
	}
	return false;
}

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
	while(attempt < 10) {
		if(Socket_Receive(tmp->sock, tmp->rdbuf, 5, MSG_PEEK) == 5) {
			if(String_CaselessCompare2(tmp->rdbuf, "GET /", 5)) {
				WebSock *wscl = Memory_Alloc(1, sizeof(WebSock));
				wscl->proto = "ClassiCube";
				wscl->recvbuf = tmp->rdbuf;
				wscl->sock = tmp->sock;
				tmp->websock = wscl;
				if(WebSock_DoHandshake(wscl))
					break;
				else attempt = 10;
			} else break;
		}
		Thread_Sleep(100);
		attempt++;
	}

	if(attempt < 10) {
		if(!AddClient(tmp)) {
			Client_Kick(tmp, Lang_Get(Lang_KickGrp, 1));
			Client_Free(tmp);
		} else Client_Loop(tmp);
	} else {
		Client_Kick(tmp, Lang_Get(Lang_KickGrp, 7));
		Client_Free(tmp);
	}

	return 0;
}

THREAD_FUNC(SockAcceptThread) {
	(void)param;
	struct sockaddr_in caddr;

	while(Server_Active) {
		Socket fd = Socket_Accept(Server_Socket, &caddr);
		if(fd == INVALID_SOCKET) break;
		if(!Server_Active) {
			Socket_Close(fd);
			break;
		}

		Client *tmp = Memory_TryAlloc(1, sizeof(Client));
		if(tmp) {
			tmp->sock = fd;
			tmp->id = CLIENT_SELF;
			tmp->mutex = Mutex_Create();
			tmp->addr = caddr.sin_addr.s_addr;
			tmp->thread = Thread_Create(ClientInitThread, tmp, false);
		} else
			Socket_Close(fd);
	}

	if(Server_Active) {
		Error_PrintSys(false);
		Server_Active = false;
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

	Log_Info(Lang_Get(Lang_ConGrp, 0), ip, port);
	if(!Socket_Bind(Server_Socket, &ssa)) {
		Error_PrintSys(true);
	}
}

cs_bool Server_Init(void) {
	if(!Socket_Init() || !Lang_Init() || !Generators_Init()) return false;

	cs_ulong zflags = zlibCompileFlags();
	if(zflags & BIT(17)) {
		Log_Error("Your zlib installation has no gzip support.");
		return false;
	}
	if(zflags & BIT(21)) {
		Log_Warn("Your zlib installation supports only one, lowest compression level!");
		Log_Warn("This means less CPU load in deflate tasks, but the worlds will take much more space on your disk.");
		Log_Warn("It also means a longer connection of players to the server.");
	}

	CStore *cfg = Config_NewStore(MAINCFG);
	CEntry *ent;

	Server_StartTime = Time_GetMSec();
	Server_Active = true;
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

	ent = Config_NewEntry(cfg, CFG_WORLDS_KEY, CFG_TSTR);
	Config_SetComment(ent, "List of worlds to load at startup. (Can be \"*\" it means load all worlds in the folder.)");
	Config_SetDefaultStr(ent, "world.cws:256x256x256:normal,flat_world.cws:256x256x256:flat");

	ent = Config_NewEntry(cfg, CFG_HEARTBEAT_KEY, CFG_TBOOL);
	Config_SetComment(ent, "Enable ClassiCube heartbeat.");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_HEARTBEATDELAY_KEY, CFG_TINT8);
	Config_SetComment(ent, "Heartbeat request delay. [1-60]");
	Config_SetLimit(ent, 1, 60);
	Config_SetDefaultInt8(ent, 10);

	ent = Config_NewEntry(cfg, CFG_HEARTBEAT_PUBLIC_KEY, CFG_TBOOL);
	Config_SetComment(ent, "Show server in the ClassiCube server list.");
	Config_SetDefaultBool(ent, false);

	cfg->modified = true;
	if(!Config_Load(cfg)) {
		Config_PrintError(cfg);
		return false;
	}
	Log_SetLevelStr(Config_GetStrByKey(cfg, CFG_LOGLEVEL_KEY));

	Broadcast = Memory_Alloc(1, sizeof(Client));
	Broadcast->mutex = Mutex_Create();
	Packet_RegisterDefault();
	Plugin_LoadAll();

	Directory_Ensure("worlds");
	cs_str worlds = Config_GetStrByKey(cfg, CFG_WORLDS_KEY);
	cs_uint32 wIndex = 0;
	if(*worlds == '*') {
		DirIter wIter;
		if(Iter_Init(&wIter, "worlds", "cws")) {
			do {
				if(wIter.isDir || !wIter.cfile) continue;
				World *tmp = World_Create(wIter.cfile);
				if(!World_Load(tmp) || !World_Add(tmp))
					World_Free(tmp);
				else wIndex++;
			} while(Iter_Next(&wIter));
		}
		Iter_Close(&wIter);

		if(wIndex < 1) {
			World *tmp = World_Create("world.cws");
			SVec defdims = {256, 256, 256};
			World_SetDimensions(tmp, &defdims);
			World_AllocBlockArray(tmp);
			if(!Generators_Use(tmp, "normal", NULL))
				Log_Error("Oh! Error happened in the world generator.");
			AList_AddField(&World_Head, tmp);
		}
	} else {
		cs_bool skip_creating = false;
		cs_byte state = 0, pos = 0;
		cs_char buffer[64];
		SVec dims = {0, 0, 0};
		World *tmp = NULL;

		do {
			if(*worlds == ':' || *worlds == ',' || *worlds == '\0') {
				buffer[pos] = '\0';

				if(state == 0) {
					skip_creating = false;
					tmp = World_Create(buffer);
					if(World_Load(tmp)) {
						Waitable_Wait(tmp->wait);
						if(World_IsReadyToPlay(tmp)) {
							AList_AddField(&World_Head, tmp);
							skip_creating = true;
							wIndex++;
						}
					}
				} else if(!skip_creating && state == 1) {
					cs_char *del = buffer, *prev = buffer;
					for(cs_uint16 i = 0; i < 3; i++) {
						del = (cs_char *)String_FirstChar(del, 'x');
						if(del) *del++ = '\0'; else if(i != 2) break;
						((cs_uint16 *)&dims)[i] = (cs_uint16)String_ToInt(prev);
						prev = del;
					}
					if(tmp && dims.x > 0 && dims.y > 0 && dims.z > 0) {
						World_SetDimensions(tmp, &dims);
						World_AllocBlockArray(tmp);
						AList_AddField(&World_Head, tmp);
						wIndex++;
					} else {
						Log_Error("Invalid dimensions specified for \"%s\"", tmp->name);
						World_Free(tmp);
					}
				} else if(!skip_creating && state == 2) {
					GeneratorRoutine gr = Generators_Get(buffer);
					if(gr) {
						if(!gr(tmp, NULL))
							Log_Error("World generator failed for \"%s\"", tmp->name);
					} else
						Log_Error("Invalid generator specified for \"%s\"", tmp->name);
				}

				if(*worlds == ':')
					state++;
				else if(*worlds == '\0')
					state = 3;
				else
					state = 0;
				pos = 0;
			} else
				buffer[pos++] = *worlds;
			worlds++;
		} while(state < 3);
	}

	if(!World_Head) {
		Log_Error("No worlds loaded.");
		return false;
	} else
		Log_Info("%d world(-s) successfully loaded.", wIndex);

	if(Config_GetBoolByKey(cfg, CFG_HEARTBEAT_KEY))
		Heartbeat_Start(Config_GetInt8ByKey(cfg, CFG_HEARTBEATDELAY_KEY));

	cs_str ip = Config_GetStrByKey(cfg, CFG_SERVERIP_KEY);
	cs_uint16 port = Config_GetInt16ByKey(cfg, CFG_SERVERPORT_KEY);
	Bind(ip, port);
	Event_Call(EVT_POSTSTART, NULL);
	if(ConsoleIO_Init())
		Log_Info(Lang_Get(Lang_ConGrp, 8));
	Thread_Create(SockAcceptThread, NULL, true);
	Server_Ready = true;
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
			Server_LatestBadTick = Time_GetMSec();
			Log_Warn(Lang_Get(Lang_ConGrp, 2));
			delta = 0;
		}
		if(delta > 500) {
			Server_LatestBadTick = Time_GetMSec();
			Log_Warn(Lang_Get(Lang_ConGrp, 1), delta);
			delta = 500;
		}
		Server_DoStep(delta);
		Thread_Sleep(1000 / TICKS_PER_SECOND);
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
