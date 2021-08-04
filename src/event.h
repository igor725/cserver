#ifndef EVENT_H
#define EVENT_H
#include <client.h>
typedef void(*evtVoidCallback)(void *);
typedef cs_bool(*evtBoolCallback)(void *);

enum {
	EVT_POSTSTART,
	EVT_ONTICK,
	EVT_ONSTOP,
	EVT_ONSPAWN,
	EVT_ONDESPAWN,
	EVT_ONHANDSHAKEDONE,
	EVT_PRELVLFIN,
	EVT_ONMESSAGE,
	EVT_ONHELDBLOCKCHNG,
	EVT_ONBLOCKPLACE,
	EVT_ONCLICK,
	EVT_ONMOVE,
	EVT_ONROTATE,
	EVT_ONDISCONNECT,
	EVT_ONWEATHER,
	EVT_ONCOLOR,

	EVENT_TYPES
};

typedef struct _onMessage {
	Client *client;
	cs_str message;
	cs_byte *type;
} onMessage;

typedef struct _onHeldBlockChange {
	Client *client;
	BlockID prev, curr;
} onHeldBlockChange;

typedef struct _onBlockPlace {
	Client *client;
	cs_byte mode;
	SVec *pos;
	BlockID *id;
} onBlockPlace;

typedef struct _onPlayerClick {
	Client *client;
	cs_int8 button, action;
	cs_int16 yaw, pitch;
	ClientID tgid;
	SVec *pos;
	cs_char face;
} onPlayerClick;

API cs_bool Event_RegisterVoid(cs_uint32 type, evtVoidCallback func);
API cs_bool Event_RegisterBool(cs_uint32 type, evtBoolCallback func);
API cs_bool Event_Unregister(cs_uint32 type, cs_uintptr evtFuncPtr);
#define EVENT_UNREGISTER(t, e) \
Event_Unregister(t, (cs_uintptr)e);

cs_bool Event_Call(cs_uint32 type, void *param);
cs_bool Event_OnMessage(Client *client, cs_char *message, cs_byte *type);
void Event_OnHeldBlockChange(Client *client, BlockID prev, BlockID curr);
cs_bool Event_OnBlockPlace(Client *client, cs_byte mode, SVec *pos, BlockID *id);
void Event_OnClick(Client *client, cs_char button, cs_char action, cs_int16 yaw, cs_int16 pitch, ClientID tgID, SVec *tgBlockPos, cs_char tgBlockFace);
#endif // EVENT_H
