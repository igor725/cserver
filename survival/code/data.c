#include <core.h>
#include <platform.h>
#include <client.h>
#include <config.h>

#include "data.h"

SURVDATA survDataList[MAX_CLIENTS] = {0};

void SurvData_Create(Client cl) {
	SURVDATA ptr = Memory_Alloc(1, sizeof(struct survivalData));
	ptr->client = cl;
	ptr->health = 20;
	ptr->oxygen = 10;
	survDataList[cl->id] = ptr;
}

void SurvData_Free(Client cl) {
	Memory_Free(survDataList[cl->id]);
	survDataList[cl->id] = NULL;
}

SURVDATA SurvData_Get(Client cl) {
	return survDataList[cl->id];
}

SURVDATA SurvData_GetByID(ClientID id) {
	return survDataList[id];
}
