#include <core.h>
#include <client.h>
#include <config.h>

#include "data.h"

SURVDATA* survDataList[MAX_CLIENTS] = {0};

void SurvData_Create(CLIENT* cl) {
	SURVDATA* ptr = (SURVDATA*)Memory_Alloc(1, sizeof(SURVDATA));
	ptr->client = cl;
	ptr->health = 10.0f;
	ptr->oxygen = 10.0f;
	survDataList[cl->id] = ptr;
}

SURVDATA* SurvData_Get(CLIENT* cl) {
	return survDataList[cl->id];
}
