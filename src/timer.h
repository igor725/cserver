#ifndef TIMER_H
#define TIMER_H
#include "core.h"

typedef void(*TimerCallback)(cs_int32, cs_int32, void *);
#define TIMER_FUNC(N) \
static void N(cs_int32 ticks, cs_int32 left, void *ud)

typedef struct _Timer {
	cs_int32 delay, nexttick, ticks, left;
	TimerCallback callback;
	void *userdata;
} Timer;

#ifndef CORE_BUILD_PLUGIN
	void Timer_RemoveAll(void);
	void Timer_Update(cs_int32 delta);
#endif

API Timer *Timer_Add(cs_int32 ticks, cs_uint32 delay, TimerCallback callback, void *ud);
API void Timer_Remove(Timer *timer);
#endif
