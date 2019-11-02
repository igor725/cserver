#include <core.h>
#include <platform.h>
#include <client.h>
#include <config.h>

#include "data.h"

void SurvData_Create(Client cl) {
	SURVDATA ptr = Memory_Alloc(1, sizeof(struct survivalData));
	Assoc_Set(cl, SurvData_AssocType, (void*)ptr);
	ptr->client = cl;
	ptr->health = 20;
	ptr->oxygen = 10;
}

void SurvData_Free(Client cl) {
	Assoc_Remove(cl, SurvData_AssocType, true);
}

SURVDATA SurvData_Get(Client cl) {
	return Assoc_GetPtr(cl, SurvData_AssocType);
}

SURVDATA SurvData_GetByID(ClientID id) {
	Client cl = Client_GetByID(id);
	return cl ? Assoc_GetPtr(cl, SurvData_AssocType) : NULL;
}
