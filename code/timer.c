#include "core.h"
#include "platform.h"
#include "timer.h"

Timer* headTimer;

Timer* Timer_Add(cs_int32 ticks, cs_uint32 delay, TimerCallback callback, void* ud) {
	Timer* timer = Memory_Alloc(1, sizeof(Timer));
	if(headTimer)
		headTimer->prev = timer;
	timer->next = headTimer;
	headTimer = timer;

	timer->left = ticks;
	timer->delay = delay;
	timer->callback = callback;
	timer->userdata = ud;

	return timer;
}

void Timer_Remove(Timer* timer) {
	if(timer->prev)
		timer->prev->next = timer->next;
	else
		headTimer = timer->next;
	if(timer->next)
		timer->next->prev = timer->prev;
	Memory_Free(timer);
}

void Timer_Update(cs_int32 delta) {
	Timer* timer = headTimer;

	while(timer) {
		timer->nexttick -= delta;
		if(timer->nexttick <= 0) {
			timer->nexttick = timer->delay;
			if(timer->left != -1) --timer->left;
			timer->callback(++timer->ticks, timer->left, timer->userdata);
			if(timer->left == 0)
				Timer_Remove(timer);
		}
		timer = timer->next;
	}
}
