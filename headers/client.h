#ifndef CLIENT_H
#define CLIENT_H
#include "world.h"
#include "websocket.h"

enum playerStates {
	STATE_MOTD,
	STATE_WLOADDONE,
	STATE_WLOADERR,
	STATE_INGAME
};

typedef struct cpeHacks {
	bool flying, noclip, speeding;
	bool spawnControl, tpv;
	short jumpHeight;
} *HACKS;

typedef struct cpeData {
	BlockID heldBlock;
	short _extCount;
	EXT headExtension;
	HACKS hacks;
	char model[16];
	const char* appName;
} *CPEDATA;

typedef struct playerData {
	const char* key;
	const char* name;
	int state;
	ANGLE* angle;
	VECTOR* position;
	bool isOP;
	bool spawned;
	WORLD world;
} *PLAYERDATA;

typedef struct client {
	ClientID id;
	uint32_t pps;
	uint32_t ppstm;
	SOCKET sock;
	uint32_t addr;
	bool closed;
	char* rdbuf;
	char* wrbuf;
	MUTEX* mutex;
	THREAD thread;
	THREAD mapThread;
	CPEDATA cpeData;
	PLAYERDATA playerData;
	WSCLIENT websock;
} *CLIENT;

void Client_SetPos(CLIENT client, VECTOR* vec, ANGLE* ang);
void Client_UpdatePositions(CLIENT client);
int  Client_Send(CLIENT client, int len);
void Client_HandshakeStage2(CLIENT client);
bool Client_CheckAuth(CLIENT client);
void Client_Disconnect(CLIENT client);
TRET Client_ThreadProc(TARG param);
void Client_Free(CLIENT client);
void Client_Tick(CLIENT client);
bool Client_Add(CLIENT client);
void Client_Init(void);

API bool Client_ChangeWorld(CLIENT client, WORLD world);
API void Client_Chat(CLIENT client, MessageType type, const char* message);
API void Client_Kick(CLIENT client, const char* reason);
API bool Client_SendMap(CLIENT client, WORLD world);

API bool Client_IsSupportExt(CLIENT client, uint32_t extCRC32, int extVer);
API bool Client_IsInSameWorld(CLIENT client, CLIENT other);
API bool Client_IsInWorld(CLIENT client, WORLD world);
API bool Client_IsInGame(CLIENT client);

API bool Client_SetWeather(CLIENT client, Weather type);
API bool Client_SetInvOrder(CLIENT client, Order order, BlockID block);
API bool Client_SetProperty(CLIENT client, uint8_t property, int value);
API bool Client_SetTexturePack(CLIENT client, const char* url);
API bool Client_SetType(CLIENT client, bool isOP);
API bool Client_SetBlock(CLIENT client, short x, short y, short z, BlockID id);
API bool Client_SetModel(CLIENT client, const char* model);
API bool Client_SetBlockPerm(CLIENT client, BlockID block, bool allowPlace, bool allowDestroy);
API bool Client_SetHeld(CLIENT client, BlockID block, bool canChange);
API bool Client_SetHotbar(CLIENT client, Order pos, BlockID block);
API bool Client_SetHacks(CLIENT client);

API bool Client_GetType(CLIENT client);
API const char* Client_GetName(CLIENT client);
API const char* Client_GetAppName(CLIENT client);
API CLIENT Client_GetByID(ClientID id);
API CLIENT Client_GetByName(const char* name);

API bool Client_Spawn(CLIENT client);
API bool Client_Despawn(CLIENT client);

VAR CLIENT Client_Broadcast;
VAR CLIENT Clients_List[MAX_CLIENTS];
#endif
