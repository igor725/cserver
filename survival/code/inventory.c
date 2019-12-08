#include <core.h>
#include <client.h>
#include <block.h>

#include "data.h"
#include "inventory.h"
#include "gui.h"

void SurvInv_Init(SurvivalData* data) {
	Client* client = data->client;

	SurvInv_UpdateInventory(data);
	for(Order i = 0; i < 9; i++) {
		Client_SetHotbar(client, i, BLOCK_AIR);
	}
}

void SurvInv_UpdateInventory(SurvivalData* data) {
	Order invIdx = 0;
	Client* client = data->client;
	cs_uint16* inv = data->inventory;

	for(BlockID i = 1; i < 255; i++) {
		cs_bool mz = inv[i] > 0 || data->godMode;
		Client_SetBlockPerm(client, i, mz, data->godMode);
		if(mz)
			Client_SetInvOrder(client, ++invIdx, i);
		else
			Client_SetInvOrder(client, 0, i);
	}
}

cs_uint16 SurvInv_Add(SurvivalData* data, BlockID id, cs_uint16 count) {
	cs_uint16* inv = data->inventory;
	cs_uint16 old = inv[id];

	if(old < SURV_MAX_BLOCKS) {
		cs_uint16 newC = min(SURV_MAX_BLOCKS, old + count);
		inv[id] = newC;
		if(old < 1)
			SurvInv_UpdateInventory(data);
		return newC - old;
	}
	return 0;
}

cs_uint16 SurvInv_Get(SurvivalData* data, BlockID id) {
	return data->inventory[id];
}

cs_uint16 SurvInv_Take(SurvivalData* data, BlockID id, cs_uint16 count) {
	cs_uint16* inv = data->inventory;
	cs_uint16 old = inv[id];

	if(old > 0) {
		cs_uint16 newC = old - min(old, count);
		inv[id] = newC;
		if(newC == 0)
			SurvInv_UpdateInventory(data);
		return old - newC;
	}
	return 0;
}
