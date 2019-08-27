#ifndef CLIENT_H
#define CLIENT_H
#include "core.h"
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
	bool cpeEnabled;
	BlockID heldBlock;
	short   _extCount;
	EXT*    firstExtension;
	EXT*    tailExtension;
	char*   appName;
} CPEDATA;

typedef struct playerData {
	char*   key;
	char*   name;
	int     state;
	ANGLE*  angle;
	VECTOR* position;
	bool    spawned;
	void*   mapThread;
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
	void*       thread;
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
int Client_ThreadProc(void* lpParam);
bool Client_SendMap(CLIENT* client);
void Client_Destroy(CLIENT* client);
bool Client_Spawn(CLIENT* client);
void Client_Tick(CLIENT* client);
ClientID Client_FindFreeID();
void Client_Init();

CLIENT* clients[256];
CLIENT* Broadcast;
#endif
