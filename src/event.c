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

#define EVENTS_FCOUNT 128

static Event *regEvents[EVENTS_TCOUNT][EVENTS_FCOUNT] = {{NULL}};

#define rgPart1 \
for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) { \
	if(!regEvents[type][pos]) { \
		Event *evt = Memory_Alloc(1, sizeof(Event));

#define rgPart2 \
		regEvents[type][pos] = evt; \
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

cs_bool Event_RegisterBunch(EventRegBunch *bunch) {
	for(cs_int32 i = 0; bunch[i].evtfunc; i++) {
		switch(bunch[i].ret) {
			case 'b':
				if(!Event_RegisterBool(bunch[i].type, (evtBoolCallback)bunch[i].evtfunc))
					return false;
				break;
			case 'v':
				if(!Event_RegisterVoid(bunch[i].type, (evtVoidCallback)bunch[i].evtfunc))
					return false;
				break;
			default:
				return false;
		}
	}

	return true;
}

cs_bool Event_Unregister(EventTypes type, cs_uintptr evtFuncPtr) {
	for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
		Event *evt = regEvents[type][pos];

		if(evt && evt->func.fptr == evtFuncPtr) {
			Memory_Free(regEvents[type][pos]);
			regEvents[type][pos] = NULL;
			return true;
		}
	}
	return false;
}

void Event_UnregisterBunch(EventRegBunch *bunch) {
	for(cs_int32 i = 0; bunch[i].evtfunc; i++)
		Event_Unregister(bunch[i].type, (cs_uintptr)bunch[i].evtfunc);
}

void Event_UnregisterAll(void) {
	for(cs_int32 type = 0; type < EVENTS_TCOUNT; type++) {
		for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
			Memory_Free(regEvents[type][pos]);
			regEvents[type][pos] = NULL;
		}
	}
}

cs_bool Event_Call(EventTypes type, void *param) {
	cs_bool ret = true;

	for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
		Event *evt = regEvents[type][pos];
		if(!evt) continue;

		if(evt->rtype == 1)
			ret = evt->func.fbool(param);
		else
			evt->func.fvoid(param);

		if(!ret) break;
	}

	return ret;
}
