#include <core.h>
#include <client.h>
#include <block.h>

#include "data.h"
#include "inventory.h"

void SurvInv_Init(SURVDATA data) {
	CLIENT client = data->client;

	for(Order i = 0; i < 9; i++) {
		Client_SetHotbar(client, i, BLOCK_AIR);
	}

	Order invIdx = 0;
	BlockID* inv = data->inventory;

	for(BlockID i = 0; i < 255; i++) {
		Client_SetBlockPerm(client, i, false, false);
		if(inv[i] > 0)
			Client_SetInvOrder(client, invIdx++, i);
		else
			Client_SetInvOrder(client, i, BLOCK_AIR);
	}
}
