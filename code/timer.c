#include "core.h"
#include "platform.h"
#include "timer.h"

typedef struct Timer {
	cs_int32 delay, nexttick, ticks, left;
	TimerCallback callback;
	void* userdata;
} Timer;

static AListField* headTimer;

AListField* Timer_Add(cs_int32 ticks, cs_uint32 delay, TimerCallback callback, void* ud) {
	Timer* timer = Memory_Alloc(1, sizeof(Timer));
	timer->left = ticks;
	timer->delay = delay;
	timer->callback = callback;
	timer->userdata = ud;
	return AList_AddField(&headTimer, timer);
}

void Timer_Remove(AListField* timer) {
	Memory_Free(timer->value);
	AList_Remove(&headTimer, timer);
}

void Timer_Update(cs_int32 delta) {
	AListField* field;
	List_Iter(field, &headTimer) {
		Timer* timer = field->value;
		timer->nexttick -= delta;
		if(timer->nexttick <= 0) {
			timer->nexttick = timer->delay;
			if(timer->left != -1) --timer->left;
			timer->callback(++timer->ticks, timer->left, timer->userdata);
			if(timer->left == 0)
				Timer_Remove(field);
		}
	}
}
