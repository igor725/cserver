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
	bool        fmSupport;
	BlockID     heldBlock;
	short       _extCount;
	EXT         headExtension;
	char        model[64];
	const char* appName;
} *CPEDATA;

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
} *PLAYERDATA;

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
	CPEDATA    cpeData;
	PLAYERDATA playerData;
} *CLIENT;

void Client_UpdateBlock(CLIENT client, WORLD world, uint16_t x, uint16_t y, uint16_t z);
void Client_ReadPos(CLIENT client, char* data, bool extended);
void Client_SetPos(CLIENT client, VECTOR* vec, ANGLE* ang);
int  Client_Send(CLIENT client, int len);
void Client_HandlePacket(CLIENT client);
void Client_HandshakeStage2(CLIENT client);
bool Client_CheckAuth(CLIENT client);
TRET Client_ThreadProc(TARG lpParam);
void Client_Free(CLIENT client);
void Client_Tick(CLIENT client);
ClientID Client_FindFreeID(void);
void Client_Init(void);

API bool Client_ChangeWorld(CLIENT client, WORLD world);
API void Client_Chat(CLIENT client, MessageType type, const char* message);
API void Client_Kick(CLIENT client, const char* reason);
API bool Client_SendMap(CLIENT client, WORLD world);

API bool Client_IsSupportExt(CLIENT client, const char* packetName, int* verPtr);
API bool Client_IsInSameWorld(CLIENT client, CLIENT other);
API bool Client_IsInWorld(CLIENT client, WORLD world);
API bool Client_IsInGame(CLIENT client);

API bool Client_SetWeather(CLIENT client, Weather type);
API bool Client_SetType(CLIENT client, bool isOP);
API bool Client_SetModel(CLIENT client, const char* model);
API bool Client_SetBlockPerm(CLIENT client, BlockID block, bool allowPlace, bool allowDestroy);
API bool Client_SetHotbar(CLIENT client, Order pos, BlockID block);

API bool Client_GetType(CLIENT client);
API const char* Client_GetName(CLIENT client);
API const char* Client_GetAppName(CLIENT client);
API CLIENT Client_GetByID(ClientID id);
API CLIENT Client_GetByName(const char* name);
void Client_Disconnect(CLIENT client);

API bool Client_Spawn(CLIENT client);
API bool Client_Despawn(CLIENT client);

CLIENT Broadcast;
CLIENT Clients_List[MAX_CLIENTS];
#endif
