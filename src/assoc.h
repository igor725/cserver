#ifndef ASSOC_H
#define ASSOC_H
#include <core.h>

typedef enum _EAssocBindType {
	ASSOC_BIND_WORLD,
	ASSOC_BIND_CLIENT
} EAssocBindType;

typedef cs_int16 AssocType;

API AssocType Assoc_NewType(EAssocBindType bindto);
API cs_bool Assoc_DelType(AssocType type);
API void *Assoc_AllocFor(void *target, AssocType type, cs_size num, cs_size size);
API void *Assoc_GetPtr(void *target, AssocType type);
API cs_bool Assoc_Remove(void *target, AssocType type);
#endif
