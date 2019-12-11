#include <core.h>
#include <str.h>
#include <log.h>
#include <client.h>
#include <event.h>
#include <block.h>
#include <command.h>
#include <protocol.h>
#include <server.h>

#include "data.h"
#include "damage.h"
#include "gui.h"
#include "hacks.h"
#include "break.h"
#include "inventory.h"

static void Survival_OnHandshake(void* param) {
	Client* client = param;
	if(!Client_GetExtVer(client, EXT_HACKCTRL) ||
	!Client_GetExtVer(client, EXT_MESSAGETYPE) ||
	!Client_GetExtVer(client, EXT_PLAYERCLICK) ||
	!Client_GetExtVer(client, EXT_HELDBLOCK)) {
		Client_Kick(client, "Your client doesn't support necessary CPE extensions.");
		return;
	}
	SurvData_Create(client);
}

static void Survival_OnSpawn(void* param) {
	SurvivalData* data = SurvData_Get((Client*)param);
	SurvGui_DrawAll(data);
	SurvHacks_Update(data);
	SurvInv_Init(data);
}

static cs_bool Survival_OnBlockPlace(void* param) {
	onBlockPlace* a = param;
	Client* client = a->client;
	SurvivalData* data = SurvData_Get(client);
	if(data->godMode) return true;

	cs_uint8 mode = a->mode;
	BlockID id = *a->id;

	if(mode == 0x00) {
		Client_Kick(client, "Your client seems to be ignoring the setBlockPermission packet.");
		return false;
	}

	if(mode == 0x01 && SurvInv_Take(data, id, 1)) {
		if(SurvInv_Get(data, id) < 1) {
			SurvGui_DrawBlockInfo(data, 0);
			return true;
		}
		SurvGui_DrawBlockInfo(data, id);
		return true;
	}

	return false;
}

static void Survival_OnHeldChange(void* param) {
	onHeldBlockChange* a = param;
	SurvivalData* data = SurvData_Get(a->client);
	if(!data->godMode)
		SurvGui_DrawBlockInfo(data, a->curr);
}

static void Survival_OnTick(void* param) {
	cs_int32 delta = *(cs_int32*)param;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		SurvivalData* data = SurvData_GetByID(i);
		if(data) {
			if(data->breakStarted)
				SurvBrk_Tick(data, delta);
			SurvDmg_Tick(data, delta);
		}
	}
}

static void Survival_OnDisconnect(void* param) {
	SurvData_Free((Client*)param);
}

static double root(double n){
  double lo = 0, hi = n, mid;
  for(cs_int32 i = 0; i < 1000; i++){
      mid = (lo + hi) / 2;
      if(mid * mid == n) return mid;
      if(mid * mid > n) hi = mid;
      else lo = mid;
  }
  return mid;
}

static float distance(float x1, float y1, float z1, float x2, float y2, float z2) {
	return (float)root((x2 * x2 - x1 * x1) + (y2 * y2 - y1 * y1) + (z2 * z2 - z1 * z1));
}

static void Survival_OnClick(void* param) {
	onPlayerClick* a = param;
	if(a->button != 0) return;

	Client* client = a->client;
	SurvivalData* data = SurvData_Get(client);
	if(data->godMode) return;

	if(a->action == 1) {
		SurvBrk_Stop(data);
		return;
	}

	SVec* pos = a->pos;
	Client* target = Client_GetByID(a->tgid);
	SurvivalData* dataTg = NULL;
	if(target) dataTg = SurvData_Get(target);

	float dist_entity = 32768.0f;
	float dist_block = 32768.0f;

	PlayerData* pd = client->playerData;
	Vec* pv = &pd->position;

	if(!Vec_IsInvalid(pos)) {
		dist_block = distance(pos->x + .5f, pos->y + .5f, pos->z + .5f, pv->x, pv->y, pv->z);
	}

	if(target) {
		Vec* pvt = &target->playerData->position;
		dist_entity = distance(pvt->x, pvt->y + 1.59375f, pvt->z, pv->x, pv->y, pv->z);
	}

	if(data->breakStarted && !SVec_Compare(&data->lastClick, pos)) {
		SurvBrk_Stop(data);
		return;
	}

	if(dist_block < dist_entity) {
		if(!data->breakStarted) {
			BlockID bid = World_GetBlock(pd->world, pos);
			if(bid > BLOCK_AIR) SurvBrk_Start(data, bid);
		}
		data->lastClick = *pos;
	} else if(dist_entity < dist_block && dist_entity < 3.5) {
		if(data->breakStarted) {
			SurvBrk_Stop(data);
			return;
		}
		if(data->pvpMode && dataTg->pvpMode) {
			SurvDmg_Hurt(dataTg, data, 1);
			// TODO: Knockback
		} else {
			if(!data->pvpMode)
				Client_Chat(client, 0, "Enable pvp mode (/pvp) first.");
		}
	}
}

COMMAND_FUNC(God) {
	SurvivalData* data = SurvData_Get(ccdata->caller);
	data->godMode ^= 1;
	SurvGui_DrawAll(data);
	SurvHacks_Update(data);
	SurvInv_UpdateInventory(data);
	SurvGui_DrawBlockInfo(data, data->godMode ? 0 : Client_GetHeldBlock(ccdata->caller));
	String_FormatBuf(ccdata->out, MAX_CMD_OUT, "God mode %s", MODE(data->godMode));
	return true;
}

COMMAND_FUNC(Hurt) {
	char damage[32];
	if(String_GetArgument(ccdata->args, damage, 32, 0)) {
		cs_uint8 dmg = (cs_uint8)(String_ToFloat(damage) * 2);
		SurvDmg_Hurt(SurvData_Get(ccdata->caller), NULL, dmg);
	}

	return false;
}

COMMAND_FUNC(PvP) {
	SurvivalData* data = SurvData_Get(ccdata->caller);
	if(data->godMode) {
		Command_Print("This command can't be used from god mode.");
	}

	data->pvpMode ^= 1;
	String_FormatBuf(ccdata->out, MAX_CMD_OUT, "PvP mode %s", MODE(data->pvpMode));
	return true;
}

Plugin_SetVersion(1)

cs_bool Plugin_Load(void) {
	if(Server_Active) {
		Log_Error("Survival plugin can be loaded only at server startup.");
		return false;
	}

	COMMAND_ADD(God, CMDF_OP | CMDF_CLIENT);
	COMMAND_ADD(Hurt, CMDF_CLIENT);
	COMMAND_ADD(PvP, CMDF_CLIENT);

	SurvData_AssocType = Assoc_NewType();
	Event_RegisterVoid(EVT_ONTICK, Survival_OnTick);
	Event_RegisterVoid(EVT_ONSPAWN, Survival_OnSpawn);
	Event_RegisterVoid(EVT_ONHELDBLOCKCHNG, Survival_OnHeldChange);
	Event_RegisterBool(EVT_ONBLOCKPLACE, Survival_OnBlockPlace);
	Event_RegisterVoid(EVT_ONDISCONNECT, Survival_OnDisconnect);
	Event_RegisterVoid(EVT_ONHANDSHAKEDONE, Survival_OnHandshake);
	Event_RegisterVoid(EVT_ONCLICK, Survival_OnClick);
	return true;
}

cs_bool Plugin_Unload(void) {
	return false;
}
