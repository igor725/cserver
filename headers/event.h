#ifndef EVENT_H
#define EVENT_H
#include "client.h"
#include "cpe.h"

typedef unsigned int ReturnType;
typedef unsigned int EventType;
typedef void(*evtVoidCallback)(void* param);
typedef bool(*evtBoolCallback)(void* param);

enum eventType {
	EVT_POSTSTART,
	EVT_ONTICK,
	EVT_ONSTOP,
	EVT_ONSPAWN,
	EVT_ONDESPAWN,
	EVT_ONHANDSHAKEDONE,
	EVT_ONMESSAGE,
	EVT_ONHELDBLOCKCHNG,
	EVT_ONBLOCKPLACE,
	EVT_ONPLAYERCLICK,
	EVT_ONDISCONNECT
};

enum eventReturn {
	EVT_RTVOID,
	EVT_RTBOOL
};

#define ETYPES     11
#define MAX_EVENTS 64

typedef struct onMessage {
	CLIENT* client;
	char* message;
	MessageType* type;
} onMessage_t;

typedef struct onHeldBlockChange {
	CLIENT* client;
	BlockID* prev;
	BlockID* curr;
} onHeldBlockChange_t;

typedef struct onBlockPlace {
	CLIENT* client;
	ushortp x, y, z;
	BlockID* id;
} onBlockPlace_t;

typedef struct onPlayerClick {
	CLIENT* client;
	char* button;
	char* action;
	short* yaw;
	short* pitch;
	ClientID* tgID;
	ushortp x, y, z;
	char face;
} onPlayerClick_t;

typedef struct event {
	ReturnType rtype;
	union {
		evtBoolCallback fbool;
		evtVoidCallback fvoid;
	} func;
} EVENT;

EVENT* Event_List[ETYPES][MAX_EVENTS];

API bool Event_RegisterBool(EventType type, evtBoolCallback func);
API bool Event_RegisterVoid(EventType type, evtVoidCallback func);

bool Event_Call(EventType type, void* param);
bool Event_OnMessage(CLIENT* client, char* message, MessageType* id);
void Event_OnHeldBlockChange(CLIENT* client, BlockID* prev, BlockID* curr);
bool Event_OnBlockPlace(CLIENT* client, ushort* x, ushort* y, ushort* z, BlockID* id);
void Event_OnClick(CLIENT* client, char* button, char* action, short* yaw, short* pitch, ClientID* tgID, ushort* tgBlockX, ushort* tgBlockY, ushort* tgblockZ, char* tgBlockFace);
#endif
