#include "core.h"
#include "str.h"
#include "csmath.h"
#include "block.h"
#include "generators.h"

#include "generators/flat.c"
#include "generators/default.c"

struct GenRoutineStruct {GeneratorRoutine func;};

cs_bool Generators_Init(void) {
	return Generators_Add("flat", flatgenerator) &&
	Generators_Add("default", defaultgenerator);
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
