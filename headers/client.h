#ifndef CLIENT_H
#define CLIENT_H
#include "world.h"

enum sockStatuses {
	CLIENT_OK,
	CLIENT_WAITCLOSE
};

enum playerStates {
	STATE_MOTD,
	STATE_WLOADDONE,
	STATE_WLOADERR,
	STATE_INGAME
};

typedef struct cpeData {
	bool    fmSupport;
	BlockID heldBlock;
	short   _extCount;
	EXT*    headExtension;
	char    model[64];
	char*   appName;
} CPEDATA;

typedef struct playerData {
	const char*   key;
	const char*   name;
	int     state;
	ANGLE*  angle;
	VECTOR* position;
	bool    isOP;
	bool    spawned;
	WORLD  world;
	bool    positionUpdated;
} PLAYERDATA;

typedef struct client {
	ClientID    id;
	SOCKET      sock;
	uint32_t    addr;
	int         status;
	char*       rdbuf;
	char*       wrbuf;
	uint16_t    bufpos;
	MUTEX*      mutex;
	THREAD      thread;
	THREAD      mapThread;
	CPEDATA*    cpeData;
	PLAYERDATA* playerData;
} *CLIENT;

void Client_UpdateBlock(CLIENT client, WORLD world, uint16_t x, uint16_t y, uint16_t z);
API bool Client_IsSupportExt(CLIENT client, const char* packetName);
API bool Client_IsInSameWorld(CLIENT client, CLIENT other);
API bool Client_IsInWorld(CLIENT client, WORLD world);
API bool Client_IsInGame(CLIENT client);
void Client_SetPos(CLIENT client, VECTOR* vec, ANGLE* ang);
API bool Client_SetWeather(CLIENT client, Weather type);
API bool Client_ChangeWorld(CLIENT client, WORLD world);
API bool Client_SetType(CLIENT client, bool isOP);
API bool Client_GetType(CLIENT client);
API bool Client_SetModel(CLIENT client, const char* model);
API bool Client_SetBlockPerm(CLIENT client, BlockID block, bool allowPlace, bool allowDestroy);
API const char* Client_GetName(CLIENT client);
API const char* Client_GetAppName(CLIENT client);
API CLIENT Client_GetByID(ClientID id);
API void Client_Chat(CLIENT client, MessageType type, const char* message);
API bool Client_SetHotbar(CLIENT client, Order pos, BlockID block);
API void Client_Kick(CLIENT client, const char* reason);
API bool Client_SendMap(CLIENT client, WORLD world);
void Client_HandshakeStage2(CLIENT client);
API CLIENT Client_FindByName(const char* name);
int  Client_Send(CLIENT client, int len);
void Client_HandlePacket(CLIENT client);
void Client_Disconnect(CLIENT client);
bool Client_CheckAuth(CLIENT client);
TRET Client_ThreadProc(TARG lpParam);
void Client_Free(CLIENT client);
API bool Client_Spawn(CLIENT client);
API bool Client_Despawn(CLIENT client);
void Client_Tick(CLIENT client);
ClientID Client_FindFreeID();
void Client_Init();

CLIENT Broadcast;
CLIENT Clients_List[MAX_CLIENTS];
#endif
