#ifndef EVENTTYPES_H
#define EVENTTYPES_H
#include "core.h"
#include "vector.h"
#include "types/block.h"
#include "types/client.h"
#include "types/world.h"

typedef void(*evtVoidCallback)(void *);
typedef cs_bool(*evtBoolCallback)(void *);

typedef enum _EventTypes {
	EVT_POSTSTART,
	EVT_ONTICK,
	EVT_ONSTOP,
	EVT_ONCONNECT,
	EVT_ONHANDSHAKEDONE,
	EVT_ONUSERTYPECHANGE,
	EVT_ONDISCONNECT,
	EVT_ONSPAWN,
	EVT_ONDESPAWN,
	EVT_ONMESSAGE,
	EVT_ONHELDBLOCKCHNG,
	EVT_ONBLOCKPLACE,
	EVT_ONCLICK,
	EVT_ONMOVE,
	EVT_ONROTATE,
	EVT_ONWEATHER,
	EVT_ONCOLOR,
	EVT_ONWORLDADDED,
	EVT_ONWORLDLOADED,
	EVT_ONWORLDUNLOADED,
	EVT_ONWORLDREMOVED,
	EVT_ONPLUGINMESSAGE,
	EVT_ONLOG,

	EVENTS_TCOUNT
} EventTypes;

typedef struct _EventRegBunch {
	cs_byte ret;
	EventTypes type;
	void *evtfunc;
} EventRegBunch;

typedef struct _onHandshakeDone {
	Client *const client;
	World *world;
} onHandshakeDone;

typedef struct _onSpawn {
	Client *const client;
	Vec *const position;
	Ang *const angle;
	cs_bool updateenv;
} onSpawn;

typedef struct _onMessage {
	Client *const client;
	cs_char *message;
	cs_byte type;
} onMessage;

typedef struct _onHeldBlockChange {
	Client *const client;
	const BlockID prev, curr;
} onHeldBlockChange;

/**
 * TODO:
 * Расставить для двух следующих
 * эвентов const спецификаторы,
 * когда предоставится возможность.
 */
typedef struct _onBlockPlace {
	Client *client;
	cs_byte mode;
	SVec pos;
	BlockID id;
} onBlockPlace;

typedef struct _onPlayerClick {
	Client *client;
	cs_int8 button, action;
	Ang angle;
	ClientID tgid;
	SVec tgpos;
	EBlockFace tgface;
} onPlayerClick;

typedef struct _onPluginMessage {
	Client *const client;
	const cs_byte channel;
	cs_char message[65];
} onPluginMessage;
#endif
