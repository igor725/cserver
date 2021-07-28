#include "core.h"
#include "str.h"
#include "generators.h"

static cs_bool flatgenerator(World *world) {
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
	return true;
}

cs_bool Generators_Init(void) {
	return Generators_Add("flat", flatgenerator);
}

cs_bool Generators_Add(cs_str name, GeneratorRoutine gr) {
	return KList_Add(&Generators_List, (void *)name, (void *)gr) != NULL;
}

cs_bool Generators_Remove(cs_str name) {
	KListField *ptr = NULL;

	List_Iter(ptr, Generators_List) {
		if(String_CaselessCompare(ptr->key.str, name)) {
			KList_Remove(&Generators_List, ptr);
			return true;
		}
	}

	return false;
}

cs_bool Generators_RemoveByFunc(GeneratorRoutine gr) {
	KListField *ptr = NULL;

	List_Iter(ptr, Generators_List) {
		if(ptr->value.ptr == gr) {
			KList_Remove(&Generators_List, ptr);
			return true;
		}
	}

	return false;
}

cs_bool Generators_Use(World *world, cs_str name) {
	KListField *ptr = NULL;

	List_Iter(ptr, Generators_List) {
		if(String_CaselessCompare(ptr->key.str, name)) {
			GeneratorRoutine gr = (GeneratorRoutine)ptr->value.ptr;
			if(gr) return gr(world);
			break;
		}
	}

	return false;
}
