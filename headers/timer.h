#ifndef TIMER_H
#define TIMER_H
#include "list.h"

#define TIMER_FUNC(N) \
static void N(cs_int32 ticks, cs_int32 left, void *ud)

typedef void(*TimerCallback)(cs_int32 ticks, cs_int32 left, void *ud);

void Timer_Update();
API AListField *Timer_Add(cs_int32 ticks, cs_uint32 delay, TimerCallback callback, void *ud);
API void Timer_Remove(AListField *timer);
#endif
