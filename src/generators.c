#include "core.h"
#include "world.h"
#include "generators.h"
#include "csmath.h"

// Генератор плоского мира

void Generator_Flat(World *world) {
	WorldInfo *wi = &world->info;
	SVec *dims = &wi->dimensions;

	BlockID *data = World_GetBlockArray(world, NULL);
	cs_int32 dirtEnd = dims->x * dims->z * (dims->y / 2 - 1);
	for(cs_int32 i = 0; i < dirtEnd + dims->x * dims->z; i++) {
		if(i < dirtEnd)
			data[i] = 3;
		else
			data[i] = 2;
	}

	World_SetProperty(world, PROP_CLOUDSLEVEL, dims->y + 2);
	World_SetProperty(world, PROP_EDGELEVEL, dims->y / 2);

	wi->spawnVec.x = (float)dims->x / 2;
	wi->spawnVec.y = (float)(dims->y / 2) + 1.59375f;
	wi->spawnVec.z = (float)dims->z / 2;
}

/*
** Генератор обычного мира.
** Когда-нибудь он точно будет
** готов, но явно не сегодня.
*/

/*
#define MAX_THREADS 16

Thread threads[MAX_THREADS];
cs_int32 cfgMaxThreads = 2;

static cs_int32 AddThread(TFUNC func, TARG arg) {
	for(cs_int32 i = 0; i < MAX_THREADS; i++) {
		if(i > cfgMaxThreads) {
			i = 0;
			if(Thread_IsValid(threads[i])) {
				Thread_Join(threads[i]);
				threads[i] = NULL;
			}
		}
		if(!Thread_IsValid(threads[i])) {
			threads[i] = Thread_Create(func, arg);
			return i;
		}
	}
	return -1;
}

static void WaitAll(void) {
	for(cs_int32 i = 0; i < MAX_THREADS; i++) {
		if(Thread_IsValid(threads[i]))
			Thread_Join(threads[i]);
	}
}

void Generator_Default(World *world) {
	RNGState rnd;
	Random_Seed(&rnd, 1337);
}
*/
