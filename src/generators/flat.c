#include "core.h"
#include "world.h"

static cs_bool flatgenerator(World *world, void *data) {
	(void)data;
	WorldInfo *wi = &world->info;
	SVec *dims = &wi->dimensions;

	BlockID *blocks = World_GetBlockArray(world, NULL);
	cs_int32 dirtEnd = dims->x * dims->z * (dims->y / 2 - 1);
	for(cs_int32 i = 0; i < dirtEnd + dims->x * dims->z; i++) {
		if(i < dirtEnd)
			blocks[i] = 3;
		else
			blocks[i] = 2;
	}

	World_SetProperty(world, PROP_CLOUDSLEVEL, dims->y + 2);
	World_SetProperty(world, PROP_EDGELEVEL, dims->y / 2);

	wi->spawnVec.x = (float)dims->x / 2;
	wi->spawnVec.y = (float)(dims->y / 2) + 1.59375f;
	wi->spawnVec.z = (float)dims->z / 2;
	return true;
}
