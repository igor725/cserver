#ifndef EVENTTYPES_H
#define EVENTTYPES_H
#include "core.h"
#include "vector.h"
#include "types/block.h"
#include "types/client.h"
#include "types/world.h"
#include "types/command.h"

typedef void(*evtVoidCallback)(void *);
typedef cs_bool(*evtBoolCallback)(void *);

typedef enum _EventType {
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
	EVT_ONPING,
	EVT_ONCLICK,
	EVT_ONMOVE,
	EVT_ONROTATE,
	EVT_ONWORLDSTATUSCHANGE,
	EVT_ONWORLDADDED,
	EVT_ONWORLDREMOVED,
	EVT_ONPLUGINLOAD,
	EVT_ONPLUGINUNLOAD,
	EVT_ONPLUGINMESSAGE,
	EVT_ONLOG,

	EVT_PRECOMMAND,
	EVT_PREHANDSHAKEDONE,
	EVT_PREWORLDENVUPDATE,

	EVENTS_TCOUNT
} EventType;

typedef struct _EventRegBunch {
	cs_byte ret;
	EventType type;
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
	cs_char message[CPE_MAX_EXTMESG_LEN];
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
	ESetBlockMode mode;
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
	cs_char message[MAX_STR_LEN];
} onPluginMessage;

typedef struct _preCommand {
	Command *const command;
	Client *const caller;
	cs_str const args;
	cs_bool allowed;
} preCommand;

typedef struct _preHandshakeDone {
	Client *const client;
	cs_char name[MAX_STR_LEN], motd[MAX_STR_LEN];
} preHandshakeDone;

typedef struct _preWorldEnvUpdate {
	World *const world;
	const cs_byte values;
	const cs_uint16 props;
	const cs_byte colors;
} preWorldEnvUpdate;
#endif
