#include <core.h>
#include <client.h>
#include <event.h>
#include <block.h>
#include <command.h>

#include "data.h"
#include "gui.h"

static void Survival_OnHandshake(void* param) {
	CLIENT* cl = (CLIENT*)param;
	SurvData_Create(cl);
}

static void Survival_OnSpawn(void* param) {
	CLIENT* cl = (CLIENT*)param;
	SURVDATA* survData = SurvData_Get(cl);
	SurvGui_DrawAll(survData);
	SurvGui_DrawBreakProgress(survData);
}

static void Survival_OnClick(void* param) {
	onPlayerClick_t* a = (onPlayerClick_t*)param;
	CLIENT* client = a->client;
	SURVDATA* survData = SurvData_Get(client);
	CLIENT* target = Client_GetByID(*a->tgID);
	SURVDATA* survDataTg = SurvData_Get(target);
}

static bool CHandler_God(const char* args, CLIENT* caller, char* out) {

}

EXP int Plugin_ApiVer = 100;
EXP bool Plugin_Init() {
	Event_RegisterVoid(EVT_ONSPAWN, Survival_OnSpawn);
	Event_RegisterVoid(EVT_ONHANDSHAKEDONE, Survival_OnHandshake);
	Event_RegisterVoid(EVT_ONPLAYERCLICK, Survival_OnClick);
	Command_Register("god", CHandler_God, true);
	return true;
}
