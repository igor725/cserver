#ifndef EVENT_H
#define EVENT_H
#include "client.h"
#include "cpe.h"

typedef cs_uint32 ReturnType;
typedef cs_uint32 EventType;
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
	EVT_ONPLAYERMOVE,
	EVT_ONPLAYERROTATE,
	EVT_ONDISCONNECT,
	EVT_ONWEATHER,
	EVT_ONCOLOR
};

enum eventReturn {
	EVT_RTVOID,
	EVT_RTBOOL
};

typedef struct _onMessage {
	Client client;
	char* message;
	MessageType* type;
} *onMessage;

typedef struct _onHeldBlockChange {
	Client client;
	BlockID prev, curr;
} *onHeldBlockChange;

typedef struct _onBlockPlace {
	Client client;
	cs_uint8 mode;
	SVec* pos;
	BlockID* id;
} *onBlockPlace;

typedef struct _onPlayerClick {
	Client client;
	cs_int8 button, action;
	cs_int16 yaw, pitch;
	ClientID id;
	SVec* pos;
	char face;
} *onPlayerClick;

typedef struct event {
	ReturnType rtype;
	union {
		evtBoolCallback fbool;
		evtVoidCallback fvoid;
	} func;
} *EVENT;

API bool Event_RegisterBool(EventType type, evtBoolCallback func);
API bool Event_RegisterVoid(EventType type, evtVoidCallback func);
API bool Event_Unregister(EventType type, void* callbackPtr);

bool Event_Call(EventType type, void* param);
bool Event_OnMessage(Client client, char* message, MessageType* type);
void Event_OnHeldBlockChange(Client client, BlockID prev, BlockID curr);
bool Event_OnBlockPlace(Client client, cs_uint8 mode, SVec* pos, BlockID* id);
void Event_OnClick(Client client, char button, char action, cs_int16 yaw, cs_int16 pitch, ClientID tgID, SVec* tgBlockPos, char tgBlockFace);
#endif
