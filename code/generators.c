#include "core.h"
#include "block.h"
#include "world.h"
#include "platform.h"
#include "generators.h"
#include "math.h"

/*
	Flat generator
*/

void Generator_Flat(WORLD* world) {
	WORLDINFO* wi = world->info;
	uint16_t dx = wi->dim->width,
	dy = wi->dim->height,
	dz = wi->dim->length;

	BlockID* data = world->data + 4;
	int dirtEnd = dx * dz * (dy / 2 - 1);
	for(int i = 0; i < dirtEnd + dx * dz; i++) {
		if(i < dirtEnd)
			data[i] = 3;
		else
			data[i] = 2;
	}

	wi->spawnVec->x = (float)dx / 2;
	wi->spawnVec->y = (float)dy / 2;
	wi->spawnVec->z = (float)dz / 2;
}

/*
	Default generator [WIP]
*/

#define MAX_THREADS 16

THREAD threads[MAX_THREADS] = {0};
int cfgMaxThreads = 2;

// Функции закоменчены, чтобы избежать варнингов
// static int AddThread(TFUNC func, TARG arg) {
// 	for(int i = 0; i < MAX_THREADS; i++) {
// 		if(i > cfgMaxThreads) {
// 			i = 0;
// 			if(Thread_IsValid(threads[i])) {
// 				Thread_Join(threads[i]);
// 				threads[i] = NULL;
// 			}
// 		}
// 		if(!Thread_IsValid(threads[i])) {
// 			threads[i] = Thread_Create(func, arg);
// 			return i;
// 		}
// 	}
// 	return -1;
// }
//
// static void WaitAll() {
// 	for(int i = 0; i < MAX_THREADS; i++) {
// 		if(Thread_IsValid(threads[i]))
// 			Thread_Join(threads[i]);
// 	}
// }

void Generator_Default(WORLD* world) {
	RNGState rnd;
	Random_Seed(&rnd, 1337);
}
