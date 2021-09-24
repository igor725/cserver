#include "core.h"
#include "platform.h"
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

cs_bool Event_RegisterVoid(EventTypes type, evtVoidCallback func) {
	rgPart1
	evt->rtype = 0;
	evt->func.fvoid = func;
	rgPart2
}

cs_bool Event_RegisterBool(EventTypes type, evtBoolCallback func) {
	rgPart1
	evt->rtype = 1;
	evt->func.fbool = func;
	rgPart2
}

cs_bool Event_Unregister(EventTypes type, cs_uintptr evtFuncPtr) {
	for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
		Event *evt = Event_List[type][pos];

		if(evt && evt->func.fptr == evtFuncPtr) {
			Event_List[type][pos] = NULL;
			return true;
		}
	}
	return false;
}

cs_bool Event_Call(EventTypes type, void *param) {
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
