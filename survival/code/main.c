#include <core.h>
#include <client.h>
#include <event.h>
#include <block.h>
#include <command.h>

#include "data.h"
#include "damage.h"
#include "gui.h"

static void Survival_OnHandshake(void* param) {
	CLIENT* cl = (CLIENT*)param;
	SurvData_Create(cl);
}

static void Survival_OnSpawn(void* param) {
	CLIENT* client = (CLIENT*)param;
	SURVDATA* survData = SurvData_Get(client);
	SurvGui_DrawAll(survData);
	for(int i = 0; i < 9; i++) {
		Client_SetHotbar(client, i, 0);
	}
}

static void Survival_OnClick(void* param) {
	onPlayerClick_t* a = (onPlayerClick_t*)param;
	CLIENT* client = a->client;
	SURVDATA* survData = SurvData_Get(client);
	CLIENT* target = Client_GetByID(*a->tgID);
	SURVDATA* survDataTg;
	if(target)
		survDataTg = SurvData_Get(target);
}

static bool CHandler_God(const char* args, CLIENT* caller, char* out) {
	Command_OnlyForOP;
	
	SURVDATA* survData = SurvData_Get(caller);
	bool mode = survData->godMode;
	survData->godMode = !mode;
	String_FormatBuf(out, CMD_MAX_OUT, "God mode %s for %s", mode ? "disabled" : "enabled", caller->playerData->name);
	return true;
}

static bool CHandler_Hurt(const char* args, CLIENT* caller, char* out) {
	char damage[32];
	if(String_GetArgument(args, damage, 32, 0)) {
		float dmg = String_ToFloat(damage);
		SurvDamage_Hurt(SurvData_Get(caller), NULL, dmg);
	}
	return false;
}

EXP int Plugin_ApiVer = 100;
EXP bool Plugin_Init() {
	Event_RegisterVoid(EVT_ONSPAWN, Survival_OnSpawn);
	Event_RegisterVoid(EVT_ONHANDSHAKEDONE, Survival_OnHandshake);
	Event_RegisterVoid(EVT_ONPLAYERCLICK, Survival_OnClick);
	Command_Register("god", CHandler_God);
	Command_Register("hurt", CHandler_Hurt);
	return true;
}
