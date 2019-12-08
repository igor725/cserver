#include <core.h>
#include <platform.h>
#include <client.h>
#include <config.h>

#include "data.h"

void SurvData_Create(Client* client) {
	SurvivalData* ptr = Memory_Alloc(1, sizeof(SurvivalData));
	Assoc_Set(client, SurvData_AssocType, (void*)ptr);
	ptr->client = client;
	ptr->health = 20;
	ptr->oxygen = 10;
}

void SurvData_Free(Client* client) {
	Assoc_Remove(client, SurvData_AssocType, true);
}

SurvivalData* SurvData_Get(Client* client) {
	return Assoc_GetPtr(client, SurvData_AssocType);
}

SurvivalData* SurvData_GetByID(ClientID id) {
	Client* client = Client_GetByID(id);
	return client ? Assoc_GetPtr(client, SurvData_AssocType) : NULL;
}
