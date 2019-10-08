#include <core.h>
#include <client.h>
#include <server.h>

#include "data.h"
#include "break.h"

static const int BreakTimings[256] = {
	0,4000,500,500,4000,1100,0,-1
};

static void UpdateBlock(WORLD world, short x, short y, short z, BlockID bid) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT cl = Client_GetByID(i);

		if(cl && Client_IsInWorld(cl, world)) Client_SetBlock(cl, x, y, z, bid);
	}
}

void SurvivalBrk_Start(SURVDATA survData, short x, short y, short z, BlockID block) {
	survData->breakStarted = true;
	survData->breakBlock = block;
	survData->breakTimer = 0;
}

void SurvivalBrk_Stop(SURVDATA survData) {
	survData->breakStarted = false;
}

void SurvivalBrk_Done(SURVDATA survData) {
	short* pos = survData->lastclick;
	short x = pos[0], y = pos[1], z = pos[2];
	WORLD world = survData->client->playerData->world;

	SurvivalBrk_Stop(survData);
	World_SetBlock(world, x, y, z, 0);
	UpdateBlock(world, x, y, z, 0);
}

void SurvivalBrk_Tick(SURVDATA survData) {
	int breakTime = BreakTimings[survData->breakBlock];
	if(breakTime == -1) {
		SurvivalBrk_Done(survData);
		return;
	} else if(breakTime == 0) {
		SurvivalBrk_Stop(survData);
		return;
	}

	survData->breakTimer += Server_Delta;
	float df = (survData->breakTimer / (float)breakTime);
	survData->breakProgress = (uint8_t)(df * SURV_MAX_BRK);
	if(survData->breakTimer >= breakTime) SurvivalBrk_Done(survData);
}
