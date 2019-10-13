#include "core.h"
#include "client.h"
#include "event.h"

EVENT Event_List[EVENT_TYPES][MAX_EVENTS] = {0};

#define rgPart1 \
for(int pos = 0; pos < MAX_EVENTS; pos++) { \
	if(!Event_List[type][pos]) { \
		EVENT evt = Memory_Alloc(1, sizeof(struct event));

#define rgPart2 \
		Event_List[type][pos] = evt; \
		return true; \
	} \
} \
return false;

bool Event_RegisterBool(EventType type, evtBoolCallback func) {
	rgPart1;
	evt->rtype = EVT_RTBOOL;
	evt->func.fbool = func;
	rgPart2;
}

bool Event_RegisterVoid(EventType type, evtVoidCallback func) {
	rgPart1;
	evt->rtype = EVT_RTVOID;
	evt->func.fvoid = func;
	rgPart2;
}

bool Event_Unregister(EventType type, void* callbackPtr) {
	for(int pos = 0; pos < MAX_EVENTS; pos++) {
		EVENT evt = Event_List[type][pos];
		if(!evt) continue;

		if(evt->func.fvoid == callbackPtr) {
			Event_List[type][pos] = NULL;
			return true;
		}
	}
	return false;
}

bool Event_Call(EventType type, void* param) {
	bool ret = true;

	for(int pos = 0; pos < MAX_EVENTS; pos++) {
		EVENT evt = Event_List[type][pos];
		if(!evt) continue;

		if(evt->rtype == EVT_RTBOOL)
			ret = evt->func.fbool(param);
		else
			evt->func.fvoid(param);

		if(!ret) break;
	}

	return ret;
}

bool Event_OnMessage(CLIENT client, char* message, MessageType* type) {
	struct onMessage params = {0};
	params.client = client;
	params.message = message;
	params.type = type;
	return Event_Call(EVT_ONMESSAGE, &params);
}

bool Event_OnBlockPlace(CLIENT client, uint8_t mode, uint16_t x, uint16_t y, uint16_t z, BlockID* id) {
	struct onBlockPlace params = {0};
	params.client = client;
	params.mode = mode;
	params.x = x;
	params.y = y;
	params.z = z;
	params.id = id;
	return Event_Call(EVT_ONBLOCKPLACE, &params);
}

void Event_OnHeldBlockChange(CLIENT client, BlockID prev, BlockID curr) {
	struct onHeldBlockChange params = {0};
	params.client = client;
	params.prev = prev;
	params.curr = curr;
	Event_Call(EVT_ONHELDBLOCKCHNG, &params);
}

void Event_OnClick(CLIENT client, char btn, char act, short yaw, short pitch, ClientID id, short x, short y, short z, char face) {
	struct onPlayerClick params = {0};
	params.client = client;
	params.button = btn;
	params.action = act;
	params.yaw = yaw;
	params.pitch = pitch;
	params.id = id;
	params.x = x;
	params.y = y;
	params.z = z;
	params.face = face;
	Event_Call(EVT_ONPLAYERCLICK, &params);
}
