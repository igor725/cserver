#include "core.h"
#include "str.h"
#include "list.h"
#include "generators.h"

static cs_bool flatgenerator(World *world, void *data);
static cs_bool normalgenerator(World *world, void *data);

#include "generators/flat.c"
#include "generators/normal.c"

static KListField *headGenerator = NULL;
struct _GenRoutineStruct {GeneratorRoutine func;};

cs_bool Generators_Init(void) {
	return Generators_Add("flat", flatgenerator) &&
	Generators_Add("normal", normalgenerator);
}

cs_bool Generators_Add(cs_str name, GeneratorRoutine gr) {
	KListField *tmp;
	List_Iter(tmp, headGenerator)
		if(String_CaselessCompare(tmp->key.str, name)) return false;

	KList_AddField(&headGenerator, (void *)name, (void *)gr);
	return true;
}

cs_bool Generators_Remove(cs_str name) {
	KListField *ptr = NULL;

	List_Iter(ptr, headGenerator) {
		if(String_CaselessCompare(ptr->key.str, name)) {
			KList_Remove(&headGenerator, ptr);
			return true;
		}
	}

	return false;
}

cs_bool Generators_RemoveByFunc(GeneratorRoutine gr) {
	KListField *ptr = NULL;

	List_Iter(ptr, headGenerator) {
		struct _GenRoutineStruct *grs = (struct _GenRoutineStruct *)&ptr->value.ptr;
		if(grs && grs->func == gr) {
			KList_Remove(&headGenerator, ptr);
			return true;
		}
	}

	return false;
}

cs_bool Generators_Use(World *world, cs_str name, void *data) {
	GeneratorRoutine gr = Generators_Get(name);
	return gr != NULL ? gr(world, data) : false;
}

GeneratorRoutine Generators_Get(cs_str name) {
	KListField *ptr = NULL;

	List_Iter(ptr, headGenerator) {
		if(String_CaselessCompare(ptr->key.str, name)) {
			struct _GenRoutineStruct *grs = (struct _GenRoutineStruct *)&ptr->value.ptr;
			if(grs) return grs->func;
			break;
		}
	}

	return NULL;
}

void Generators_UnregisterAll(void) {
	while(headGenerator)
		KList_Remove(&headGenerator, headGenerator);
}
