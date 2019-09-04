#include "core.h"
#include "world.h"
#include "generators.h"

/*
	Flat generator
*/

void Generator_Flat(WORLD* world) {
	WORLDINFO* wi = world->info;
	ushort dx = wi->dim->width,
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

void Generator_Default(WORLD* world) {

}
