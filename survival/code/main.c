#include <core.h>
#include <event.h>
#include <block.h>

#include "data.h"

static void Survival_OnHandshake(void* param) {
	CLIENT* cl = (CLIENT*)param;
	SurvData_Create(cl);
}

static void Survival_OnSpawn(void* param) {
	CLIENT* cl = (CLIENT*)param;
	SURVDATA* survData = SurvData_Get(cl);
	
}

EXP int Plugin_ApiVer = 100;
EXP bool Plugin_Init() {
	Event_RegisterVoid(EVT_ONSPAWN, Survival_OnSpawn);
	Event_RegisterVoid(EVT_ONHANDSHAKEDONE, Survival_OnHandshake);
	return true;
}
