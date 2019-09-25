#include <core.h>
#include <client.h>
#include <event.h>
#include <block.h>
#include <command.h>

#include "data.h"
#include "damage.h"
#include "gui.h"
#include "break.h"

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

static float fsquare(float a) {
	return a * a;
}

static double root(double n){
  double lo = 0, hi = n, mid;
  for(int i = 0; i < 1000; i++){
      mid = (lo + hi) / 2;
      if(mid * mid == n) return mid;
      if(mid * mid > n) hi = mid;
      else lo = mid;
  }
  return mid;
}

static float distance(float x1, float y1, float z1, float x2, float y2, float z2) {
	return (float)root(fsquare(x2 - x1) + fsquare(y2 - y1) + fsquare(z2 - z1));
}

static void Survival_OnClick(void* param) {
	onPlayerClick_t* a = (onPlayerClick_t*)param;
	CLIENT* client = a->client;
	short x = *a->x, y = *a->y, z = *a->z;
	SURVDATA* survData = SurvData_Get(client);
	CLIENT* target = Client_GetByID(*a->tgID);
	SURVDATA* survDataTg;
	if(target) survDataTg = SurvData_Get(target);

	if(*a->button != 0) return;

	if(*a->action == 1) {
		SurvivalBrk_Stop(survData);
		return;
	}

	float dist_entity = 32768.0f;
	float dist_block = 32768.0f;

	VECTOR* pv = client->playerData->position;

	if(x != -1 && y != -1 && z != -1) {
		dist_block = distance(x + .5f, y + .5f, z + .5f, pv->x, pv->y, pv->z);
	} else if(target) {
		VECTOR* pvt = target->playerData->position;
		dist_entity = distance(pvt->x, pvt->y, pvt->z, pv->x, pv->y, pv->z);
	}

	if(dist_block < dist_entity) {
		SurvivalBrk_Start(survData, x, y, z);
	} else if(dist_entity < dist_block && dist_entity < 3.5) {
		SurvivalInt_Start(survData, survDataTg);
	}
}

static bool CHandler_God(const char* args, CLIENT* caller, char* out) {
	Command_OnlyForClient;
	Command_OnlyForOP;

	SURVDATA* survData = SurvData_Get(caller);
	bool mode = survData->godMode;
	survData->godMode = !mode;
	String_FormatBuf(out, CMD_MAX_OUT, "God mode %s for %s", mode ? "disabled" : "enabled", caller->playerData->name);

	return true;
}

static bool CHandler_Hurt(const char* args, CLIENT* caller, char* out) {
	Command_OnlyForClient;

	char damage[32];
	if(String_GetArgument(args, damage, 32, 0)) {
		float dmg = String_ToFloat(damage);
		SurvDamage_Hurt(SurvData_Get(caller), NULL, dmg);
	}

	return false;
}

EXP int Plugin_ApiVer = 100;
EXP bool Plugin_Load() {
	Event_RegisterVoid(EVT_ONSPAWN, Survival_OnSpawn);
	Event_RegisterVoid(EVT_ONHANDSHAKEDONE, Survival_OnHandshake);
	Event_RegisterVoid(EVT_ONPLAYERCLICK, Survival_OnClick);
	Command_Register("god", CHandler_God);
	Command_Register("hurt", CHandler_Hurt);
	return true;
}
EXP bool Plugin_Unload() {
	return false;
}
