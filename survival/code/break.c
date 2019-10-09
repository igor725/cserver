#include <core.h>
#include <client.h>
#include <server.h>

#include "data.h"
#include "break.h"
#include "gui.h"

static const int BreakTimings[256] = {
	0,4000,500,500,4000,1100,0,-1
};

static void UpdateBlock(WORLD world, short x, short y, short z, BlockID bid) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT cl = Client_GetByID(i);
		if(cl && Client_IsInWorld(cl, world)) Client_SetBlock(cl, x, y, z, bid);
	}
}

void SurvBrk_Start(SURVDATA data, short x, short y, short z, BlockID block) {
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
	short* pos = data->lastclick;
	short x = pos[0], y = pos[1], z = pos[2];
	WORLD world = data->client->playerData->world;

	SurvBrk_Stop(data);
	World_SetBlock(world, x, y, z, 0);
	UpdateBlock(world, x, y, z, 0);
}

void SurvBrk_Tick(SURVDATA data) {
	int breakTime = BreakTimings[data->breakBlock];
	if(breakTime == -1) {
		SurvBrk_Done(data);
		return;
	} else if(breakTime == 0) {
		SurvBrk_Stop(data);
		return;
	}

	data->breakTimer += Server_Delta;
	float df = (data->breakTimer / (float)breakTime);
	uint8_t newProgress = (uint8_t)(df * SURV_MAX_BRK);
	if(newProgress > data->breakProgress) {
		data->breakProgress = newProgress;
		SurvGui_DrawBreakProgress(data);
	}
	if(data->breakTimer >= breakTime) SurvBrk_Done(data);
}
