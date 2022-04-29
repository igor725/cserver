#include "core.h"
#include "platform.h"
#include "event.h"

typedef struct {
	cs_uint32 rtype;
	union {
		evtBoolCallback fbool;
		evtVoidCallback fvoid;
		void *fptr;
	} func;
} Event;

#define EVENTS_FCOUNT 128

static Event regEvents[EVENTS_TCOUNT][EVENTS_FCOUNT] = {{0, NULL}};

#define rgPart1 \
for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) { \
	Event *evt = &regEvents[type][pos]; \
	if(evt->func.fptr) continue;

#define rgPart2 \
	return true; \
} \
return false;

cs_bool Event_RegisterVoid(EventType type, evtVoidCallback func) {
	rgPart1
	evt->rtype = 0;
	evt->func.fvoid = func;
	rgPart2
}

cs_bool Event_RegisterBool(EventType type, evtBoolCallback func) {
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

cs_bool Event_Unregister(EventType type, void *evtFuncPtr) {
	for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
		Event *evt = &regEvents[type][pos];

		if(evt && evt->func.fptr == evtFuncPtr) {
			regEvents[type][pos].func.fptr = NULL;
			return true;
		}
	}
	return false;
}

void Event_UnregisterBunch(EventRegBunch *bunch) {
	for(cs_int32 i = 0; bunch[i].evtfunc; i++)
		Event_Unregister(bunch[i].type, bunch[i].evtfunc);
}

void Event_UnregisterAll(void) {
	for(cs_int32 type = 0; type < EVENTS_TCOUNT; type++) {
		for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
			regEvents[type][pos].func.fptr = NULL;
		}
	}
}

cs_bool Event_Call(EventType type, void *param) {
	cs_bool ret = true;

	for(cs_int32 pos = 0; pos < EVENTS_FCOUNT; pos++) {
		Event *evt = &regEvents[type][pos];
		if(!evt->func.fptr) continue;

		if(evt->rtype == 1)
			ret = evt->func.fbool(param);
		else
			evt->func.fvoid(param);

		if(!ret) break;
	}

	return ret;
}
