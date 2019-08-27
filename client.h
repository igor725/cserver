#ifndef CLIENT_H
#define CLIENT_H
#include "core.h"
#include "world.h"
#include "winsock2.h"

#define CLIENT_OK         0
#define CLIENT_WAITCLOSE  1
#define CLIENT_AFTERCLOSE 2

#define STATE_MOTD         0
#define STATE_WLOADDONE    1
#define STATE_WLOADERR     2
#define STATE_INGAME       3

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

bool Client_IsSupportExt(CLIENT* client, const char* packetName);
void Client_SetPos(CLIENT* client, VECTOR* vec, ANGLE* ang);
void Client_Kick(CLIENT* client, const char* reason);
int  AcceptClients_ThreadProc(void* lpParam);
int  Client_Send(CLIENT* client, int len);
void Client_HandlePacket(CLIENT* client);
void Client_Disconnect(CLIENT* client);
bool Client_CheckAuth(CLIENT* client);
bool Client_SendMap(CLIENT* client);
void Client_Destroy(CLIENT* client);
bool Client_Spawn(CLIENT* client);
void Client_Tick(CLIENT* client);
void Client_Init();

CLIENT* clients[256];
void* listenThread;
CLIENT* Broadcast;
#endif
