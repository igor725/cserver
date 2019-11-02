#include <core.h>
#include <client.h>

#include "data.h"
#include "break.h"
#include "gui.h"
#include "inventory.h"

static const int32_t BreakTimings[256] = {
	0,4000,500,500,4000,1100,0,-1
};

static void UpdateBlock(World world, SVec* pos, BlockID bid) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client cl = Client_GetByID(i);
		if(cl && Client_IsInWorld(cl, world)) Client_SetBlock(cl, pos, bid);
	}
}

void SurvBrk_Start(SURVDATA data, BlockID block) {
	data->breakStarted = true;
	data->breakBlock = block;
	data->breakTimer = 0;
}

void SurvBrk_Stop(SURVDATA data) {
	data->breakProgress = 0;
	data->breakStarted = false;
	SurvGui_DrawBreakProgress(data);
}

void SurvBrk_Done(SURVDATA data) {
	SVec* pos = &data->lastClick;
	Client client = data->client;
	World world = client->playerData->world;
	BlockID id = data->breakBlock;

	SurvInv_Add(data, id, 1);
	Client_SetHeld(client, id, false);
	SurvGui_DrawBlockInfo(data, id);
	World_SetBlock(world, pos, 0);
	UpdateBlock(world, pos, 0);
	SurvBrk_Stop(data);
}

void SurvBrk_Tick(SURVDATA data, uint32_t delta) {
	int32_t breakTime = BreakTimings[data->breakBlock];
	if(breakTime == -1) {
		SurvBrk_Stop(data);
		return;
	} else if(breakTime == 0) {
		SurvBrk_Done(data);
		return;
	}

	data->breakTimer += (int16_t)delta;
	float df = (data->breakTimer / (float)breakTime);
	uint8_t newProgress = (uint8_t)(df * SURV_MAX_BRK);
	if(newProgress > data->breakProgress) {
		data->breakProgress = newProgress;
		SurvGui_DrawBreakProgress(data);
	}
	if(data->breakTimer >= breakTime) SurvBrk_Done(data);
}
