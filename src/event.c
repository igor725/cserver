#include "core.h"
#include "platform.h"
#include "client.h"
#include "event.h"

typedef struct {
	cs_uint32 rtype;
	union {
		evtBoolCallback fbool;
		evtVoidCallback fvoid;
		cs_uintptr fptr;
	} func;
} Event;

Event *Event_List[EVENTS_TCOUNT][EVENTS_FCOUNT];

#define rgPart1 \
for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) { \
	if(!Event_List[type][pos]) { \
		Event *evt = Memory_Alloc(1, sizeof(Event));

#define rgPart2 \
		Event_List[type][pos] = evt; \
		return true; \
	} \
} \
return false;

cs_bool Event_RegisterVoid(cs_uint32 type, evtVoidCallback func) {
	rgPart1
	evt->rtype = 0;
	evt->func.fvoid = func;
	rgPart2
}

cs_bool Event_RegisterBool(cs_uint32 type, evtBoolCallback func) {
	rgPart1
	evt->rtype = 1;
	evt->func.fbool = func;
	rgPart2
}

cs_bool Event_Unregister(cs_uint32 type, cs_uintptr evtFuncPtr) {
	for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
		Event *evt = Event_List[type][pos];

		if(evt && evt->func.fptr == evtFuncPtr) {
			Event_List[type][pos] = NULL;
			return true;
		}
	}
	return false;
}

cs_bool Event_Call(cs_uint32 type, void *param) {
	cs_bool ret = true;

	for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
		Event *evt = Event_List[type][pos];
		if(!evt) continue;

		if(evt->rtype == 1)
			ret = evt->func.fbool(param);
		else
			evt->func.fvoid(param);

		if(!ret) break;
	}

	return ret;
}

cs_bool Event_OnMessage(Client *client, cs_char *message, cs_byte *type) {
	onMessage params = {
		.client = client,
		.message = message,
		.type = type
	};
	return Event_Call(EVT_ONMESSAGE, &params);
}

cs_bool Event_OnBlockPlace(Client *client, cs_byte mode, SVec *pos, BlockID *id) {
	onBlockPlace params = {
		.client = client,
		.mode = mode,
		.pos = pos,
		.id = id
	};
	return Event_Call(EVT_ONBLOCKPLACE, &params);
}

void Event_OnHeldBlockChange(Client *client, BlockID prev, BlockID curr) {
	onHeldBlockChange params = {
		.client = client,
		.prev = prev,
		.curr = curr
	};
	Event_Call(EVT_ONHELDBLOCKCHNG, &params);
}

void Event_OnClick(Client *client, cs_char btn, cs_char act, cs_int16 yaw, cs_int16 pitch, ClientID id, SVec *pos, cs_char face) {
	onPlayerClick params = {
		.client = client,
		.button = btn,
		.action = act,
		.yaw = yaw,
		.pitch = pitch,
		.tgid = id,
		.pos = pos,
		.face = face
	};
	Event_Call(EVT_ONCLICK, &params);
}
