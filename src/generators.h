#ifndef GENERATORS_H
#define GENERATORS_H
#include "list.h"
#include "world.h"

typedef cs_bool(*GeneratorRoutine)(World *);
KListField *Generators_List;

cs_bool Generators_Init(void);
API cs_bool Generators_Add(cs_str name, GeneratorRoutine gr);
API cs_bool Generators_Remove(cs_str name);
API cs_bool Generators_RemoveByFunc(GeneratorRoutine gr);
API cs_bool Generators_Use(World *world, cs_str name);
#endif // GENERATORS_H
