#include "core.h"
#include "str.h"
#include "log.h"
#include "cserror.h"
#include "server.h"
#include "client.h"
#include "protocol.h"
#include "config.h"
#include "generators.h"
#include "platform.h"
#include "event.h"
#include "plugin.h"
#include "timer.h"
#include "consoleio.h"
#include "strstor.h"
#include "command.h"
#include "heartbeat.h"

CStore *Server_Config = NULL;
cs_str Server_Version = GIT_COMMIT_TAG;
cs_bool Server_Active = false, Server_Ready = false;
cs_uint64 Server_StartTime = 0;
Socket Server_Socket = 0;
Waitable *SyncClients = NULL;

THREAD_FUNC(ClientInitThread) {
	Client *client = (Client *)param;
	Waitable_Reset(client->waitend);
	cs_byte attempt = 0;
	while(attempt < 10) {
		if(Socket_Receive(client->sock, client->rdbuf, 5, MSG_PEEK) == 5) {
			if(String_CaselessCompare2(client->rdbuf, "GET /", 5)) {
				WebSock *wscl = Memory_Alloc(1, sizeof(WebSock));
				wscl->proto = "ClassiCube";
				wscl->recvbuf = client->rdbuf;
				wscl->sock = client->sock;
				client->websock = wscl;
				if(WebSock_DoHandshake(wscl))
					break;
				else attempt = 10;
			} else break;
		}
		Thread_Sleep(100);
		attempt++;
	}

	if(attempt < 10) {
		while(!client->closed) {
			Waitable_Wait(SyncClients);
			if(!client->closed)
				Client_Tick(client);
		}

		if(Client_CheckState(client, PLAYER_STATE_INGAME)) {
			for(int i = 0; i < MAX_CLIENTS; i++) {
				Client *other = Clients_List[i];
				if(other && other != client && Client_GetExtVer(other, EXT_PLAYERLIST) == 2)
					CPE_WriteRemoveName(other, client);
			}
			Client_Despawn(client);
			Event_Call(EVT_ONDISCONNECT, client);
		}
	} else
		Client_Kick(client, Sstor_Get("KICK_PERR_HS"));

	Waitable_Signal(client->waitend);
	return 0;
}

INL static ClientID TryToGetIDFor(Client *client) {
	cs_int8 maxConnPerIP = Config_GetInt8ByKey(Server_Config, CFG_CONN_KEY),
	sameAddrCount = 1;
	ClientID possibleId = CLIENT_SELF;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(!other && possibleId == CLIENT_SELF) possibleId = i;
		if(other && other->addr == client->addr)
			++sameAddrCount;
		else continue;

		if(sameAddrCount > maxConnPerIP) {
			Client_Kick(client, Sstor_Get("KICK_MANYCONN"));
			return CLIENT_SELF;
		}
	}

	return possibleId;
}

INL static void WaitAllClientThreads(void) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client) Waitable_Wait(client->waitend);
	}
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
			tmp->mutex = Mutex_Create();
			tmp->waitend = Waitable_Create();
			tmp->addr = htonl(caddr.sin_addr.s_addr);
			tmp->id = TryToGetIDFor(tmp);
			Waitable_Signal(tmp->waitend);
			if(tmp->id != CLIENT_SELF) {
				Clients_List[tmp->id] = tmp;
				Thread_Create(ClientInitThread, tmp, true);
			} else {
				Client_Kick(tmp, Sstor_Get("KICK_FULL"));
				Client_Free(tmp);
			}
		} else
			Socket_Close(fd);
	}

	if(Server_Active) {
		Error_PrintSys(false);
		Server_Active = false;
	}

	return 0;
}

INL static cs_bool Bind(cs_str ip, cs_uint16 port) {
	Server_Socket = Socket_New();
	if(!Server_Socket) {
		Error_PrintSys(true);
	}

	struct sockaddr_in ssa;
	return Socket_SetAddr(&ssa, ip, port) > 0 &&
	Socket_Bind(Server_Socket, &ssa);
}

cs_bool Server_Init(void) {
	if(!Generators_Init() || !Sstor_Defaults()) return false;

	CStore *cfg = Config_NewStore(MAINCFG);
	CEntry *ent;

	Server_StartTime = Time_GetMSec();
	Server_Active = true;
	Server_Config = cfg;

	ent = Config_NewEntry(cfg, CFG_SERVERIP_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, "Bind server to specified IP address. \"0.0.0.0\" - means \"all available network adapters\"");
	Config_SetDefaultStr(ent, "0.0.0.0");

	ent = Config_NewEntry(cfg, CFG_SERVERPORT_KEY, CONFIG_TYPE_INT16);
	Config_SetComment(ent, "Use specified port to accept clients. [1-65535]");
	Config_SetLimit(ent, 1, 65535);
	Config_SetDefaultInt16(ent, 25565);

	ent = Config_NewEntry(cfg, CFG_SERVERNAME_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, "Server name and MOTD will be shown to the player during map loading");
	Config_SetDefaultStr(ent, "Server name");

	ent = Config_NewEntry(cfg, CFG_SERVERMOTD_KEY, CONFIG_TYPE_STR);
	Config_SetDefaultStr(ent, "Server MOTD");

	ent = Config_NewEntry(cfg, CFG_LOGLEVEL_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, "I - Info, C - Chat, W - Warnings, D - Debug");
	Config_SetDefaultStr(ent, "ICWD");

	ent = Config_NewEntry(cfg, CFG_LOCALOP_KEY, CONFIG_TYPE_BOOL);
	Config_SetComment(ent, "Any player with ip address \"127.0.0.1\" will automatically become an operator");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_MAXPLAYERS_KEY, CONFIG_TYPE_INT8);
	Config_SetComment(ent, "Max players on server. [1-126]");
	Config_SetLimit(ent, 1, 126);
	Config_SetDefaultInt8(ent, 10);

	ent = Config_NewEntry(cfg, CFG_CONN_KEY, CONFIG_TYPE_INT8);
	Config_SetComment(ent, "Max connections per one IP. [1-5]");
	Config_SetLimit(ent, 1, 5);
	Config_SetDefaultInt8(ent, 5);

	ent = Config_NewEntry(cfg, CFG_WORLDS_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, "List of worlds to load at startup. (Can be \"*\" it means load all worlds in the folder)");
	Config_SetDefaultStr(ent, "world.cws:256x256x256:normal,flat_world.cws:64x64x64:flat");

	cfg->modified = true;
	if(!Config_Load(cfg)) {
		cs_int32 line = 0;
		ECExtra extra = CONFIG_EXTRA_NOINFO;
		ECError code = Config_PopError(cfg, &extra, &line);
		if(extra == CONFIG_EXTRA_IO_LINEASERROR) {
			if(line != ENOENT) {
				Log_Error(Sstor_Get("SV_CFG_ERR2"), "open", MAINCFG, Config_ErrorToString(code), extra);
				return false;
			}
		} else {
			cs_str scode = Config_ErrorToString(code), 
			sextra = Config_ExtraToString(extra);
			if(line > 0)
				Log_Error(Sstor_Get("SV_CFGL_ERR"), line, MAINCFG, scode, sextra);
			else
				Log_Error(Sstor_Get("SV_CFG_ERR"), "parse", MAINCFG, scode, sextra);

			return false;
		}
	}
	Log_SetLevelStr(Config_GetStrByKey(cfg, CFG_LOGLEVEL_KEY));
	Config_Save(Server_Config);

	SyncClients = Waitable_Create();
	Broadcast = Memory_Alloc(1, sizeof(Client));
	Broadcast->mutex = Mutex_Create();
	Broadcast->id = CLIENT_SELF;
	Command_RegisterDefault();
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
				if(World_Load(tmp)) {
					World_Lock(tmp, 0);
					if(World_HasError(tmp)) {
						EWorldExtra extra = WORLD_EXTRA_NOINFO;
						EWorldError code = World_PopError(tmp, &extra);
						Log_Error(Sstor_Get("SV_WLOAD_ERR"), "load", World_GetName(tmp), code, extra);
						World_FreeBlockArray(tmp);
						World_Free(tmp);
					} else {
						World_Add(tmp);
						wIndex++;
					}
					World_Unlock(tmp);
				} else World_Free(tmp);
			} while(Iter_Next(&wIter));
		}
		Iter_Close(&wIter);

		if(wIndex < 1) {
			cs_str wname = "world.cws";
			World *tmp = World_Create(wname);
			SVec defdims = {256, 256, 256};
			World_SetDimensions(tmp, &defdims);
			World_AllocBlockArray(tmp);
			if(!Generators_Use(tmp, "normal", NULL))
				Log_Error(Sstor_Get("WGEN_ERROR"), wname);
			World_Add(tmp);
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
						World_Lock(tmp, 0);
						if(World_HasError(tmp)) {
							EWorldExtra extra = WORLD_EXTRA_NOINFO;
							EWorldError code = World_PopError(tmp, &extra);
							if(code != WORLD_ERROR_IOFAIL && extra != WORLD_EXTRA_IO_OPEN) {
								skip_creating = true;
								Log_Error(Sstor_Get("SV_WLOAD_ERR"), "load", World_GetName(tmp), code, extra);
								World_FreeBlockArray(tmp);
								World_Free(tmp);
							}
						} else {
							if(World_IsReadyToPlay(tmp)) {
								skip_creating = true;
								World_Add(tmp);
								wIndex++;
							}
						}
						World_Unlock(tmp);
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
						World_Add(tmp);
						wIndex++;
					} else {
						Log_Error(Sstor_Get("WGEN_INVDIM"), tmp->name);
						World_Free(tmp);
					}
				} else if(!skip_creating && state == 2) {
					GeneratorRoutine gr = Generators_Get(buffer);
					if(gr) {
						if(!gr(tmp, NULL))
							Log_Error(Sstor_Get("WGEN_ERROR"), tmp->name);
					} else
						Log_Error(Sstor_Get("WGEN_NOGEN"), tmp->name);
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
		Log_Error(Sstor_Get("SV_NOWORLDS"));
		return false;
	} else
		Log_Info(Sstor_Get("SV_WLDONE"), wIndex);

	cs_str ip = Config_GetStrByKey(cfg, CFG_SERVERIP_KEY);
	cs_uint16 port = Config_GetInt16ByKey(cfg, CFG_SERVERPORT_KEY);
	if(Bind(ip, port)) {
		Log_Info(Sstor_Get("SV_START"), ip, port);
		Event_Call(EVT_POSTSTART, NULL);
		if(ConsoleIO_Init())
			Log_Info(Sstor_Get("SV_STOPNOTE"));
		Thread_Create(SockAcceptThread, NULL, true);
		Server_Ready = true;
		return true;
	}

	Log_Error(Sstor_Get("SV_BIND_FAIL"), port);
	return false;
}

INL static void DoStep(cs_int32 delta) {
	Waitable_Reset(SyncClients);
	Event_Call(EVT_ONTICK, &delta);
	Timer_Update(delta);
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client && client->closed) {
			Clients_List[client->id] = NULL;
			Waitable_Wait(client->waitend);
			Mutex_Lock(client->mutex);
			Mutex_Unlock(client->mutex);
			Client_Free(client);
		}
	}
	Waitable_Signal(SyncClients);
}

void Server_StartLoop(void) {
	if(!Server_Active) return;
	cs_uint64 last, curr = Time_GetMSec();
	cs_int32 delta;

	while(Server_Active) {
		last = curr;
		curr = Time_GetMSec();
		delta = (cs_int32)(curr - last);
		if(curr < last) {
			Log_Warn(Sstor_Get("SV_BADTICK_BW"));
			delta = 0;
		} else if(delta > 500) {
			Log_Warn(Sstor_Get("SV_BADTICK"), delta);
			delta = 500;
		}
		DoStep(delta);
		Thread_Sleep(1000 / TICKS_PER_SECOND);
	}

	Event_Call(EVT_ONSTOP, NULL);
}

INL static void UnloadAllWorlds(void) {
	AListField *tmp;
	cs_int32 attempt = 0;

	while((tmp = World_Head) != NULL) {
		World *world = (World *)tmp->value.ptr;
		cs_str wname = World_GetName(world);

		if(World_Save(world)) {
			World_WaitAllTasks(world);
			World_Lock(world, 0);
			if(World_HasError(world)) {
				EWorldExtra extra = WORLD_EXTRA_NOINFO;
				EWorldError code = World_PopError(world, &extra);
				Log_Error(Sstor_Get("SV_WLOAD_ERR"), "save", wname, code, extra);
				World_Unlock(world);
			} else {
				Event_Call(EVT_ONWORLDREMOVED, world);
				AList_Remove(&World_Head, tmp);
				World_Free(world);
				attempt = 0;
				continue;
			}
		}

		if(attempt > 10) {
			AList_Remove(&World_Head, tmp);
			World_Free(world);
		} else {
			Thread_Sleep(500);
			attempt++;
		}
	}
}

void Server_Cleanup(void) {
	ConsoleIO_Uninit();
	Log_Info(Sstor_Get("SV_STOP_PL"));
	Clients_KickAll(Sstor_Get("KICK_STOP"));
	WaitAllClientThreads();
	if(Broadcast && Broadcast->mutex) Mutex_Free(Broadcast->mutex);
	if(Broadcast) Memory_Free(Broadcast);
	Log_Info(Sstor_Get("SV_STOP_SW"));
	UnloadAllWorlds();
	Socket_Close(Server_Socket);
	Log_Info(Sstor_Get("SV_STOP_SC"));
	Config_Save(Server_Config);
	Config_DestroyStore(Server_Config);
	Log_Info(Sstor_Get("SV_STOP_UP"));
	Plugin_UnloadAll(true);
	Packet_UnregisterAll();
	Command_UnregisterAll();
	Generators_UnregisterAll();
	Event_UnregisterAll();
	Heartbeat_StopAll();
	Timer_RemoveAll();

	// Должна всегда вызываться последней
	Sstor_Cleanup();
}
