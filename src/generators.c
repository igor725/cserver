#include "core.h"
#include "str.h"
#include "generators.h"

struct GenRoutineStruct {GeneratorRoutine func;};

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

cs_bool Generators_Init(void) {
	return Generators_Add("flat", flatgenerator);
}

cs_bool Generators_Add(cs_str name, GeneratorRoutine gr) {
	struct GenRoutineStruct *grs;
	grs = (struct GenRoutineStruct *)Memory_Alloc(1, sizeof(struct GenRoutineStruct));
	grs->func = gr;
	return KList_Add(&Generators_List, (void *)name, (void *)grs) != NULL;
}

cs_bool Generators_Remove(cs_str name) {
	KListField *ptr = NULL;

	List_Iter(ptr, Generators_List) {
		if(String_CaselessCompare(ptr->key.str, name)) {
			if(ptr->value.ptr) Memory_Free(ptr->value.ptr);
			KList_Remove(&Generators_List, ptr);
			return true;
		}
	}

	return false;
}

cs_bool Generators_RemoveByFunc(GeneratorRoutine gr) {
	KListField *ptr = NULL;

	List_Iter(ptr, Generators_List) {
		struct GenRoutineStruct *grs = (struct GenRoutineStruct *)ptr->value.ptr;
		if(grs && grs->func == gr) {
			Memory_Free(ptr->value.ptr);
			KList_Remove(&Generators_List, ptr);
			return true;
		}
	}

	return false;
}

cs_bool Generators_Use(World *world, cs_str name, void *data) {
	KListField *ptr = NULL;

	List_Iter(ptr, Generators_List) {
		if(String_CaselessCompare(ptr->key.str, name)) {
			struct GenRoutineStruct *grs = (struct GenRoutineStruct *)ptr->value.ptr;
			if(grs) return grs->func(world, data);
			break;
		}
	}

	return false;
}
