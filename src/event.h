#ifndef EVENT_H
#define EVENT_H
#include "core.h"
#include "client.h"

typedef void(*evtVoidCallback)(void *);
typedef cs_bool(*evtBoolCallback)(void *);

typedef enum _EventTypes {
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

	EVENTS_TCOUNT
} EventTypes;

typedef struct _EventRegBunch {
	cs_byte ret;
	EventTypes type;
	void *evtfunc;
} EventRegBunch;

#define EVENTS_FCOUNT 128

typedef struct _onMessage {
	Client *client;
	cs_char *message;
	cs_byte type;
} onMessage;

typedef struct _onHeldBlockChange {
	Client *client;
	BlockID prev, curr;
} onHeldBlockChange;

typedef struct _onBlockPlace {
	Client *client;
	cs_byte mode;
	SVec pos;
	BlockID id;
} onBlockPlace;

typedef struct _onPlayerClick {
	Client *client;
	cs_int8 button, action;
	cs_int16 yaw, pitch;
	ClientID tgid;
	SVec tgpos;
	cs_char tgface;
} onPlayerClick;

API cs_bool Event_RegisterVoid(EventTypes type, evtVoidCallback func);
API cs_bool Event_RegisterBool(EventTypes type, evtBoolCallback func);
API cs_bool Event_RegisterBunch(EventRegBunch *bunch);
API cs_bool Event_Unregister(EventTypes type, cs_uintptr evtFuncPtr);
API cs_bool Event_UnregisterBunch(EventRegBunch *bunch);

#define EVENT_UNREGISTER(t, e) \
Event_Unregister(t, (cs_uintptr)e);

NOINL cs_bool Event_Call(EventTypes type, void *param);
#endif // EVENT_H
