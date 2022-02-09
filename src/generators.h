#ifndef GENERATORS_H
#define GENERATORS_H
#include "core.h"
#include "list.h"
#include "world.h"

typedef cs_bool(*GeneratorRoutine)(World *, void *);

cs_bool Generators_Init(void);
void Generators_UnregisterAll(void);
API cs_bool Generators_Add(cs_str name, GeneratorRoutine gr);
API cs_bool Generators_Remove(cs_str name);
API cs_bool Generators_RemoveByFunc(GeneratorRoutine gr);
API cs_bool Generators_Use(World *world, cs_str name, void *data);
API GeneratorRoutine Generators_Get(cs_str name);
#endif // GENERATORS_H
