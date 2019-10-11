#include <core.h>
#include <client.h>
#include <block.h>

#include "data.h"
#include "inventory.h"
#include "gui.h"

void SurvInv_Init(SURVDATA data) {
	CLIENT client = data->client;

	SurvInv_UpdateInventory(data);
	for(Order i = 0; i < 9; i++) {
		Client_SetHotbar(client, i, BLOCK_AIR);
	}
}

void SurvInv_UpdateInventory(SURVDATA data) {
	Order invIdx = 0;
	CLIENT client = data->client;
	uint16_t* inv = data->inventory;

	for(BlockID i = 0; i < 255; i++) {
		bool mz = inv[i] > 0;
		Client_SetBlockPerm(client, i, mz, false);
		if(mz)
			Client_SetInvOrder(client, ++invIdx, i);
		else
			Client_SetInvOrder(client, 0, i);
	}
}

uint16_t SurvInv_Add(SURVDATA data, BlockID id, uint16_t count) {
	uint16_t* inv = data->inventory;
	uint16_t old = inv[id];

	if(old < SURV_MAX_BLOCKS) {
		uint16_t newC = min(SURV_MAX_BLOCKS, old + count);
		inv[id] = newC;
		if(old < 1)
			SurvInv_UpdateInventory(data);
		return newC - old;
	}
	return 0;
}

uint16_t SurvInv_Get(SURVDATA data, BlockID id) {
	return data->inventory[id];
}

uint16_t SurvInv_Take(SURVDATA data, BlockID id, uint16_t count) {
	uint16_t* inv = data->inventory;
	uint16_t old = inv[id];

	if(old > 0) {
		uint16_t newC = old - min(old, count);
		inv[id] = newC;
		if(newC == 0)
			SurvInv_UpdateInventory(data);
		return old - newC;
	}
	return 0;
}
