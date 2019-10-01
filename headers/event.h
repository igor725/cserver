#ifndef EVENT_H
#define EVENT_H
#include "client.h"
#include "cpe.h"

typedef uint32_t ReturnType;
typedef uint32_t EventType;
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
	EVT_ONDISCONNECT,
	EVT_ONWEATHER
};

enum eventReturn {
	EVT_RTVOID,
	EVT_RTBOOL
};

typedef struct onMessage {
	CLIENT client;
	char* message;
	MessageType* type;
} onMessage_t;

typedef struct onHeldBlockChange {
	CLIENT client;
	BlockID* prev;
	BlockID* curr;
} onHeldBlockChange_t;

typedef struct onBlockPlace {
	CLIENT client;
	uint16_t *x, *y, *z;
	BlockID* id;
} onBlockPlace_t;

typedef struct onPlayerClick {
	CLIENT client;
	char* button;
	char* action;
	short* yaw;
	short* pitch;
	ClientID* tgID;
	short *x, *y, *z;
	char face;
} onPlayerClick_t;

typedef struct event {
	ReturnType rtype;
	union {
		evtBoolCallback fbool;
		evtVoidCallback fvoid;
	} func;
} *EVENT;

EVENT Event_List[EVENT_TYPES][MAX_EVENTS];

API bool Event_RegisterBool(EventType type, evtBoolCallback func);
API bool Event_RegisterVoid(EventType type, evtVoidCallback func);
API bool Event_Unregister(EventType type, void* callbackPtr);

bool Event_Call(EventType type, void* param);
bool Event_OnMessage(CLIENT client, char* message, MessageType* id);
void Event_OnHeldBlockChange(CLIENT client, BlockID* prev, BlockID* curr);
bool Event_OnBlockPlace(CLIENT client, uint16_t* x, uint16_t* y, uint16_t* z, BlockID* id);
void Event_OnClick(CLIENT client, char* button, char* action, short* yaw, short* pitch, ClientID* tgID, short* tgBlockX, short* tgBlockY, short* tgblockZ, char* tgBlockFace);
#endif
