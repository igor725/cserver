#include "core.h"
#include "str.h"
#include "log.h"
#include "cserror.h"
#include "server.h"
#include "netbuffer.h"
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
#include "types/websock.h"
#include "world.h"
#include "list.h"

CStore *Server_Config = NULL;
cs_str Server_Version = GIT_COMMIT_TAG;
cs_bool Server_Active = false, Server_Ready = false;
cs_uint64 Server_StartTime = 0;
Socket Server_Socket = 0;

INL static ClientID TryToGetIDFor(Client *client) {
	cs_int16 maxPlayers = (cs_byte)Config_GetInt16ByKey(Server_Config, CFG_MAXPLAYERS_KEY);
	cs_int8 maxConnPerIP = Config_GetInt8ByKey(Server_Config, CFG_CONN_KEY),
	sameAddrCount = 1, playersCount = 0;
	ClientID possibleId = CLIENT_SELF;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(!other && possibleId == CLIENT_SELF) possibleId = i;

		if(other) {
			if(!Client_IsBot(other)) playersCount++;
			if(other->addr == client->addr)
				sameAddrCount++;
			else continue;

			if(playersCount >= maxPlayers)
				return CLIENT_SELF;

			if(sameAddrCount >= maxConnPerIP) {
				Client_Kick(client, Sstor_Get("KICK_MANYCONN"));
				return CLIENT_SELF;
			}
		}
	}

	return possibleId;
}

INL static cs_bool ProcessClient(Client *client) {
	if(client->netbuf.closed) {
		if(client->state >= CLIENT_STATE_INGAME) {
			for(int i = 0; i < MAX_CLIENTS; i++) {
				Client *other = Clients_List[i];
				if(other && other != client && Client_GetExtVer(other, EXT_PLAYERLIST) == 2)
					CPE_WriteRemoveName(other, client);
			}
			Client_Despawn(client);
		}
		Event_Call(EVT_ONDISCONNECT, client);
		Clients_List[client->id] = NULL;
		Client_Free(client);
		return false;
	}

	if(Client_IsBot(client)) return true;
	NetBuffer_Process(&client->netbuf);

	switch(client->state) {
		case CLIENT_STATE_INITIAL:
			if(NetBuffer_AvailRead(&client->netbuf) >= 5) {
				client->state = CLIENT_STATE_MOTD;
				if(String_CaselessCompare2(NetBuffer_PeekRead(&client->netbuf, 5), "GET /", 5)) {
					client->websock = Memory_TryAlloc(1, sizeof(WebSock));
					if(!client->websock) {
						Client_Kick(client, Sstor_Get("KICK_INT"));
						return true;
					}
					client->websock->proto = "ClassiCube";
					client->websock->maxpaylen = 32 * 1024;
				}
			}
			break;

		case CLIENT_STATE_MOTD:
		case CLIENT_STATE_INGAME:
			Client_Tick(client);
			break;
	}

	return true;
}

static cs_bool ProcessClients(void) {
	cs_bool canFinish = true;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		canFinish = false;

		if(!ProcessClient(client)) continue;
		if(Client_IsBot(client)) continue;
		if(client->kickReason) continue;
		cs_uint64 currtime = Time_GetMSec(), timeout = 0;
		switch(client->state) {
			case CLIENT_STATE_INITIAL:
				timeout = 800;
				break;
			case CLIENT_STATE_MOTD:
				if(client->mapData.world)
					timeout = (cs_uint64)-1;
				else
					timeout = 800;
				break;
			case CLIENT_STATE_INGAME:
				timeout = 30000;
				break;
		}

		if(currtime - client->lastmsg > timeout) {
			client->kickReason = String_AllocCopy(Sstor_Get("KICK_TIMEOUT"));
			NetBuffer_ForceClose(&client->netbuf);
		}
	}

	return canFinish;
}

static void DoNetTick(void) {
	struct sockaddr_in caddr;
	Socket fd = Socket_Accept(Server_Socket, &caddr);
	if(fd != INVALID_SOCKET) {
		if(!Server_Active) {
			Socket_Close(fd);
			return;
		}

		Socket_SetNonBlocking(fd, true);
		Client *tmp = Memory_TryAlloc(1, sizeof(Client));
		if(tmp) {
			tmp->mutex = Mutex_Create();
			tmp->addr = caddr.sin_addr.s_addr;
			NetBuffer_Init(&tmp->netbuf, fd);
			tmp->id = TryToGetIDFor(tmp);
			if(tmp->id != CLIENT_SELF) {
				tmp->lastmsg = Time_GetMSec();
				if(Event_Call(EVT_ONCONNECT, tmp)) {
					Clients_List[tmp->id] = tmp;
					return;
				} else
					Client_Kick(tmp, Sstor_Get("KICK_REJ"));
			} else
				Client_Kick(tmp, Sstor_Get("KICK_FULL"));

			/* TODO:
			 * Возможно тут может произойти вход
			 * в бесконечный цикл (ненавижу TCP/IP),
			 * нужно разобраться с этой фигнёй как
			 * можно более изящно.
			 */
			while(NetBuffer_Process(&tmp->netbuf));
			Client_Free(tmp);
		}

		Socket_Close(fd);
	}

	ProcessClients();
}

INL static cs_bool Bind(cs_str ip, cs_uint16 port) {
	Server_Socket = Socket_New();
	if(!Server_Socket) Error_PrintSys(true);

	struct sockaddr_in ssa;
	return Socket_SetAddr(&ssa, ip, port) > 0 &&
	Socket_SetNonBlocking(Server_Socket, true) &&
	Socket_Bind(Server_Socket, &ssa);
}

cs_bool Server_Init(void) {
	if(!Generators_Init() || !Sstor_Defaults()) return false;

	Directory_Ensure("worlds");
	Directory_Ensure("configs");

	CStore *cfg = Config_NewStore(MAINCFG);
	CEntry *ent;

	Server_StartTime = Time_GetMSec();
	Server_Active = true;
	Server_Config = cfg;

	ent = Config_NewEntry(cfg, CFG_SERVERIP_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, "Bind server to specified IP address. \"0.0.0.0\" - means \"all available network adapters\"");
	Config_SetDefaultStr(ent, "0.0.0.0");

	ent = Config_NewEntry(cfg, CFG_SERVERPORT_KEY, CONFIG_TYPE_INT32);
	Config_SetComment(ent, "Use specified port to accept clients. [1-65535]");
	Config_SetLimit(ent, 1, 65535);
	Config_SetDefaultInt32(ent, 25565);

	ent = Config_NewEntry(cfg, CFG_SERVERNAME_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, "Server name and MOTD will be shown to the player during map loading");
	Config_SetDefaultStr(ent, "Server name");

	ent = Config_NewEntry(cfg, CFG_SERVERMOTD_KEY, CONFIG_TYPE_STR);
	Config_SetDefaultStr(ent, "Server MOTD");

	ent = Config_NewEntry(cfg, CFG_LOGLEVEL_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, "I - Info, C - Chat, W - Warnings, D - Debug, c - Console colors");
	Config_SetDefaultStr(ent, "ICWD");

	ent = Config_NewEntry(cfg, CFG_SANITIZE_KEY, CONFIG_TYPE_BOOL);
	Config_SetComment(ent, "Check nicknames for prohibited characters");
	Config_SetDefaultBool(ent, true);

	ent = Config_NewEntry(cfg, CFG_LOCALOP_KEY, CONFIG_TYPE_BOOL);
	Config_SetComment(ent, "Any player with ip address \"127.0.0.1\" will automatically become an operator");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_MAXPLAYERS_KEY, CONFIG_TYPE_INT16);
	Config_SetComment(ent, "Max players on server. [1-254]");
	Config_SetLimit(ent, 1, 254);
	Config_SetDefaultInt16(ent, 10);

	ent = Config_NewEntry(cfg, CFG_CONN_KEY, CONFIG_TYPE_INT8);
	Config_SetComment(ent, "Max connections per one IP. [1-5]");
	Config_SetLimit(ent, 1, 5);
	Config_SetDefaultInt8(ent, 5);

	ent = Config_NewEntry(cfg, CFG_WORLDS_KEY, CONFIG_TYPE_STR);
	Config_SetComment(ent, "List of worlds to load at startup. (Can be \"*\" it means load all worlds in the folder)");
	Config_SetDefaultStr(ent, "world:256x256x256:normal,flat_world:64x64x64:flat");

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
	Config_Save(Server_Config, true);
	Command_RegisterDefault();
	Packet_RegisterDefault();
	Plugin_LoadAll();

	cs_str worlds = Config_GetStrByKey(cfg, CFG_WORLDS_KEY);
	cs_uint32 wIndex = 0;
	if(*worlds == '*') {
		DirIter wIter;
		if(Iter_Init(&wIter, "worlds", "cws")) {
			do {
				if(wIter.isDir || !wIter.cfile) continue;
				cs_char wname[64] = {0};
				if(String_Copy(wname, 64, wIter.cfile)) {
					cs_char *ext = String_FindSubstr(wname, ".cws");
					if(ext) *ext = '\0';
				} else continue;

				World *tmp = World_Create(wname);
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
			cs_str wname = "world";
			World *tmp = World_Create(wname);
			SVec defdims = {256, 256, 256};
			World_SetDimensions(tmp, &defdims);
			World_AllocBlockArray(tmp);
			if(!Generators_Use(tmp, "normal", GENERATOR_SEED_FROM_TIME))
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
					cs_char *ext = String_FindSubstr(buffer, ".cws");
					if(ext) *ext = '\0';
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
						if(!gr(tmp, GENERATOR_SEED_FROM_TIME))
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
		Server_Ready = true;
		return true;
	}

	Log_Error(Sstor_Get("SV_BIND_FAIL"), ip, port);
	return false;
}

cs_uint64 prev, this = 0;

INL static void DoStep(cs_int32 delta) {
	(void)DoNetTick();
	(void)Event_Call(EVT_ONTICK, &delta);
	Timer_Update(delta);
}

void Server_StartLoop(void) {
	if(!Server_Active) return;
	cs_uint64 start = Time_GetMSec(), end;
	cs_int32 delta = 0;

	while(Server_Active) {
		end = start;
		start = Time_GetMSec();
		delta = (cs_int32)(start - end);
		DoStep(delta);

		if(start < end) {
			Log_Warn(Sstor_Get("SV_BADTICK_BW"));
			delta = 0;
		} else if(delta > 500) {
			Log_Warn(Sstor_Get("SV_BADTICK"), delta);
			delta = 500;
		}

		Thread_Sleep(TICKS_PER_SECOND);
	}

	Event_Call(EVT_ONSTOP, NULL);
}

INL static void UnloadAllWorlds(void) {
	AListField *tmp;
	cs_int32 attempt = 0;

	while((tmp = World_Head) != NULL) {
		World *world = (World *)tmp->value.ptr;
		cs_str wname = World_GetName(world);

		if(!World_IsInMemory(world) && World_Save(world)) {
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
			Thread_Sleep(1000);
			attempt++;
		}
	}
}

static void KickAll(cs_str reason) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client) Client_Kick(client, reason);
	}
}

cs_str Server_GetAppName(void) {
	return SOFTWARE_FULLNAME;
}

void Server_Cleanup(void) {
	ConsoleIO_Uninit();
	Log_Info(Sstor_Get("SV_STOP_PL"));
	KickAll(Sstor_Get("KICK_STOP"));
	while(!ProcessClients());
	Log_Info(Sstor_Get("SV_STOP_SW"));
	UnloadAllWorlds();
	Socket_Close(Server_Socket);
	Log_Info(Sstor_Get("SV_STOP_SC"));
	Config_Save(Server_Config, true);
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
