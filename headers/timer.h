#ifndef TIMER_H
#define TIMER_H

#define TIMER_FUNC(N) \
static void N(cs_int32 ticks, cs_int32 left, void* ud)

typedef void(*TimerCallback)(cs_int32 ticks, cs_int32 left, void* ud);

typedef struct Timer {
	cs_int32 delay, nexttick, ticks, left;
	TimerCallback callback;
	void* userdata;

	struct Timer* next;
	struct Timer* prev;
} Timer;

void Timer_Update();
API Timer* Timer_Add(cs_int32 ticks, cs_uint32 delay, TimerCallback callback, void* ud);
API void Timer_Remove(Timer* timer);
#endif
