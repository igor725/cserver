#ifndef CLIENT_H
#define CLIENT_H
#include "world.h"

enum SockStatuses {
	CLIENT_OK,
	CLIENT_WAITCLOSE,
	CLIENT_AFTERCLOSE
};

enum States {
	STATE_MOTD,
	STATE_WLOADDONE,
	STATE_WLOADERR,
	STATE_INGAME
};

typedef struct cpeData {
	bool fmSupport;
	BlockID heldBlock;
	short   _extCount;
	EXT*    headExtension;
	char*   appName;
} CPEDATA;

typedef struct playerData {
	const char*   key;
	const char*   name;
	int     state;
	ANGLE*  angle;
	VECTOR* position;
	bool    spawned;
	THREAD  mapThread;
	WORLD*  currentWorld;
	bool    positionUpdated;
} PLAYERDATA;

typedef struct client {
	ClientID    id;
	SOCKET      sock;
	int         status;
	char*       rdbuf;
	char*       wrbuf;
	ushort      bufpos;
	THREAD      thread;
	CPEDATA*    cpeData;
	PLAYERDATA* playerData;
} CLIENT;

void Client_UpdateBlock(CLIENT* client, WORLD* world, ushort x, ushort y, ushort z);
bool Client_IsSupportExt(CLIENT* client, const char* packetName);
void Client_SetPos(CLIENT* client, VECTOR* vec, ANGLE* ang);
void Client_Kick(CLIENT* client, const char* reason);
CLIENT* Client_FindByName(const char* name);
int  Client_Send(CLIENT* client, int len);
void Client_HandlePacket(CLIENT* client);
void Client_Disconnect(CLIENT* client);
bool Client_CheckAuth(CLIENT* client);
THRET Client_ThreadProc(TARG lpParam);
bool Client_SendMap(CLIENT* client);
void Client_Destroy(CLIENT* client);
bool Client_Spawn(CLIENT* client);
void Client_Tick(CLIENT* client);
ClientID Client_FindFreeID();
void Client_Init();

CLIENT* clients[256];
CLIENT* Broadcast;
#endif
