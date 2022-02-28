#include "core.h"
#include "platform.h"
#include "timer.h"
#include "list.h"

static AListField *headTimer = NULL;

Timer *Timer_Add(cs_int32 ticks, cs_uint32 delay, TimerCallback callback, void *ud) {
	Timer *timer = Memory_Alloc(1, sizeof(Timer));
	timer->left = ticks;
	timer->delay = delay;
	timer->callback = callback;
	timer->userdata = ud;
	AList_AddField(&headTimer, timer);
	return timer;
}

INL static void removeTimerByField(AListField *field) {
	if(field->value.ptr) {
		Memory_Free(field->value.ptr);
		AList_Remove(&headTimer, field);
	}
}

void Timer_Remove(Timer *timer) {
	AListField *field;
	List_Iter(field, headTimer) {
		if(timer == field->value.ptr)
			removeTimerByField(field);
	}
}

void Timer_Update(cs_int32 delta) {
	AListField *field;
	List_Iter(field, headTimer) {
		Timer *timer = field->value.ptr;
		timer->nexttick -= delta;
		if(timer->nexttick <= 0) {
			timer->nexttick = timer->delay;
			if(timer->left != -1) --timer->left;
			timer->callback(++timer->ticks, timer->left, timer->userdata);
			if(timer->left == 0)
				removeTimerByField(field);
		}
	}
}

void Timer_RemoveAll(void) {
	while(headTimer) {
		Memory_Free(headTimer->value.ptr);
		AList_Remove(&headTimer, headTimer);
	}
}
