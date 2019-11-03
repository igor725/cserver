#include "core.h"
#include "str.h"
#include "server.h"
#include "client.h"
#include "packets.h"
#include "config.h"
#include "console.h"
#include "command.h"
#include "websocket.h"
#include "generators.h"
#include "heartbeat.h"
#include "platform.h"
#include "event.h"
#include "cplugin.h"
#include "lang.h"

static void AcceptFunc(void) {
	struct sockaddr_in caddr;
	Socket fd = Socket_Accept(Server_Socket, &caddr);

	if(fd != INVALID_SOCKET) {
		if(!Server_Active) {
			Socket_Close(fd);
			return;
		}

		uint32_t addr = htonl(caddr.sin_addr.s_addr);
	 	Client tmp = Client_New(fd, addr);
		uint32_t sameAddrCount = 1;
		uint8_t maxConnPerIP = Config_GetInt8(Server_Config, CFG_CONN_KEY);
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client cc = Clients_List[i];
			if(cc && cc->addr == addr)
				++sameAddrCount;
			else continue;

			if(sameAddrCount > maxConnPerIP) {
				Client_Kick(tmp, Lang_Get(LANG_SVMANYCONN));
				Client_Free(tmp);
				return;
			}
		}

		if(Socket_Receive(fd, tmp->rdbuf, 5, MSG_PEEK)) {
			if(String_CaselessCompare(tmp->rdbuf, "GET /")) {
				WsClient wscl = Memory_Alloc(1, sizeof(struct wsClient));
				wscl->recvbuf = tmp->rdbuf;
				wscl->sock = tmp->sock;
				tmp->websock = wscl;

				if(!WsClient_DoHandshake(wscl)) {
					Client_Free(tmp);
					return;
				}
			}
		}

		if(!Client_Add(tmp)) {
			Client_Kick(tmp, Lang_Get(LANG_KICKSVFULL));
			Client_Free(tmp);
		}
	}
}

static TRET AcceptThreadProc(TARG param) {
	(void)param;
	while(Server_Active) AcceptFunc();
	return 0;
}

static void Bind(const char* ip, uint16_t port) {
	Server_Socket = Socket_New();
	struct sockaddr_in ssa;
	switch (Socket_SetAddr(&ssa, ip, port)) {
		case 0:
			Error_Print2(ET_SERVER, EC_INVALIDIP, true);
			break;
		case -1:
			Error_PrintSys(true);
			break;
	}

	Client_Init();
	Log_Info(Lang_Get(LANG_SVSTART), SOFTWARE_FULLNAME, ip, port);
	if(!Socket_Bind(Server_Socket, &ssa)) {
		Error_PrintSys(true);
	}
}

void Server_InitialWork(void) {
	if(!Socket_Init()) return;

	Log_Info(Lang_Get(LANG_SVLOADING), MAINCFG);
	CFGStore cfg = Config_NewStore(MAINCFG);
	CFGEntry ent;

	ent = Config_NewEntry(cfg, CFG_SERVERIP_KEY, CFG_STR);
	Config_SetComment(ent, "Bind server to specified IP address. \"0.0.0.0\" - means \"all available network adapters\".");
	Config_SetDefaultStr(ent, "0.0.0.0");

	ent = Config_NewEntry(cfg, CFG_SERVERPORT_KEY, CFG_INT16);
	Config_SetComment(ent, "Use specified port to accept clients. [1-65535]");
	Config_SetLimit(ent, 1, 65535);
	Config_SetDefaultInt16(ent, 25565);

	ent = Config_NewEntry(cfg, CFG_SERVERNAME_KEY, CFG_STR);
	Config_SetComment(ent, "Server name and MOTD will be shown to the player during map loading.");
	Config_SetDefaultStr(ent, "Server name");

	ent = Config_NewEntry(cfg, CFG_SERVERMOTD_KEY, CFG_STR);
	Config_SetDefaultStr(ent, "Server MOTD");

	ent = Config_NewEntry(cfg, CFG_LOGLEVEL_KEY, CFG_STR);
	Config_SetComment(ent, "I - Info, C - Chat, W - Warnings, D - Debug.");
	Config_SetDefaultStr(ent, "ICWD");

	ent = Config_NewEntry(cfg, CFG_LOCALOP_KEY, CFG_BOOL);
	Config_SetComment(ent, "Any player with ip address \"127.0.0.1\" will automatically become an operator.");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_MAXPLAYERS_KEY, CFG_INT8);
	Config_SetComment(ent, "Max players on server. [1-127]");
	Config_SetLimit(ent, 1, 127);
	Config_SetDefaultInt8(ent, 10);

	ent = Config_NewEntry(cfg, CFG_CONN_KEY, CFG_INT8);
	Config_SetComment(ent, "Max connections per one IP.");
	Config_SetLimit(ent, 1, 5);
	Config_SetDefaultInt8(ent, 5);

	ent = Config_NewEntry(cfg, CFG_HEARTBEAT_KEY, CFG_BOOL);
	Config_SetComment(ent, "Enable ClassiCube heartbeat.");
	Config_SetDefaultBool(ent, false);

	ent = Config_NewEntry(cfg, CFG_HEARTBEATDELAY_KEY, CFG_INT);
	Config_SetComment(ent, "Heartbeat request delay.");
	Config_SetDefaultInt(ent, 10);

	ent = Config_NewEntry(cfg, CFG_HEARTBEAT_PUBLIC_KEY, CFG_BOOL);
	Config_SetComment(ent, "Show server in the ClassiCube server list.");
	Config_SetDefaultBool(ent, false);

	cfg->modified = true;
	if(!Config_Load(cfg)) {
		Config_PrintError(cfg);
		Process_Exit(1);
	}
	Log_SetLevelStr(Config_GetStr(cfg, CFG_LOGLEVEL_KEY));

	Packet_RegisterDefault();
	Packet_RegisterCPEDefault();
	Command_RegisterDefault();

	Directory_Ensure("worlds");
	int32_t wIndex = 0;
	dirIter wIter = {0};
	if(Iter_Init(&wIter, "worlds", "cws")) {
		do {
			if(wIter.isDir || !wIter.cfile) continue;
			World tmp = World_Create(wIter.cfile);
			tmp->id = wIndex++;
			if(!World_Load(tmp) || !World_Add(tmp))
				World_Free(tmp);
		} while(Iter_Next(&wIter) && wIndex < MAX_WORLDS);
	}
	Iter_Close(&wIter);

	if(wIndex < 1) {
		World tmp = World_Create("world.cws");
		SVec defdims = {256, 256, 256};
		World_SetDimensions(tmp, &defdims);
		World_AllocBlockArray(tmp);
		Generator_Flat(tmp);
		Worlds_List[0] = tmp;
	}

	Log_Info(Lang_Get(LANG_SVPLUGINLOAD));
	CPlugin_Start();
	if(Config_GetBool(cfg, CFG_HEARTBEAT_KEY)) {
		Log_Info(Lang_Get(LANG_HBEAT));
		Heartbeat_Start(Config_GetInt(cfg, CFG_HEARTBEATDELAY_KEY));
	}
	Event_Call(EVT_POSTSTART, NULL);
	const char* ip = Config_GetStr(cfg, CFG_SERVERIP_KEY);
	uint16_t port = Config_GetUInt16(cfg, CFG_SERVERPORT_KEY);
	Thread_Create(AcceptThreadProc, NULL, true);
	Server_StartTime = Time_GetMSec();
	Server_Active = true;
	Server_Config = cfg;
	Console_Start();
	Bind(ip, port);
}

void Server_DoStep(void) {
	Event_Call(EVT_ONTICK, NULL);
	for(int32_t i = 0; i < MAX_CLIENTS; i++) {
		Client client = Clients_List[i];
		if(client) Client_Tick(client);
	}
}

void Server_StartLoop(void) {
	uint64_t last, curr = Time_GetMSec();
	while(Server_Active) {
		last = curr;
		curr = Time_GetMSec();
		Server_Delta = (int32_t)(curr - last);
		if(Server_Delta < 0) {
			Log_Warn(Lang_Get(LANG_SVDELTALT0));
			Server_Delta = 0;
		}
		if(Server_Delta > 500) {
			Log_Warn(Lang_Get(LANG_SVLONGTICK), Server_Delta);
			Server_Delta = 500;
		}
		Server_DoStep();
		Sleep(10);
	}
	Log_Info(Lang_Get(LANG_SVLOOPDONE));
}

void Server_Stop(void) {
	Event_Call(EVT_ONSTOP, NULL);
	Log_Info(Lang_Get(LANG_SVSTOP0));
	Clients_KickAll(Lang_Get(LANG_KICKSVSTOP));
	Log_Info(Lang_Get(LANG_SVSTOP1));
	Worlds_SaveAll(true);

	Socket_Close(Server_Socket);
	Log_Info(Lang_Get(LANG_SVSAVING), MAINCFG);
	Config_Save(Server_Config);
	Config_DestroyStore(Server_Config);

	Log_Info(Lang_Get(LANG_SVSTOP2));
	CPlugin_Stop();
}
