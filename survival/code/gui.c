#include <core.h>
#include <client.h>
#include <packets.h>
#include <block.h>
#include <str.h>

#include "data.h"
#include "gui.h"
#include "inventory.h"

void SurvGui_DrawHealth(SURVDATA data) {
	char healthstr[20] = {0};

	if(!data->godMode) {
		uint8_t hltf = data->health / 2;
		uint8_t empty = 10 - hltf;

		String_Append(healthstr, 20, "&c");
		for(int32_t i = 0; i < hltf; i++) {
			String_Append(healthstr, 20, "\3");
		}
		if(data->health % 2) {
			String_Append(healthstr, 20, "&4\3");
			empty -= 1;
		}
		if(empty > 0) {
			String_Append(healthstr, 20, "&8");
			for(int32_t i = 0; i < empty; i++) {
				String_Append(healthstr, 20, "\3");
			}
		}
	}

	Client_Chat(data->client, CPE_STATUS1, healthstr);
}

void SurvGui_DrawOxygen(SURVDATA data) {
	char oxystr[13] = {0};

	if(!data->godMode && data->showOxygen) {
		String_Copy(oxystr, 13, "&b");
		for(uint8_t i = 0; i < 10; i++) {
			String_Append(oxystr, 13, data->oxygen > i ? "\7" : " ");
		}
	}

	Client_Chat(data->client, CPE_STATUS2, oxystr);
}

void SurvGui_DrawBreakProgress(SURVDATA data) {
	char breakstr[19] = {0};

	if(data->breakStarted) {
		String_Append(breakstr, 19, "[&a");
		for(int32_t i = 0; i < 10; i++) {
			if(i == data->breakProgress)
				String_Append(breakstr, 19, "&8");
			String_Append(breakstr, 19, "|");
		}
		String_Append(breakstr, 19, "&f]");
	}

	Client_Chat(data->client, CPE_ANNOUNCE, breakstr);
}

void SurvGui_DrawBlockInfo(SURVDATA data, BlockID id) {
	char blockinfo[64] = {0};

	if(id > BLOCK_AIR) {
		const char* bn = Block_GetName(id);
		uint16_t bc = SurvInv_Get(data, id);
		String_FormatBuf(blockinfo, 64, "%s (%d)", bn, bc);
	}

	Client_Chat(data->client, CPE_BRIGHT1, blockinfo);
}

void SurvGui_DrawAll(SURVDATA data) {
	SurvGui_DrawHealth(data);
	SurvGui_DrawOxygen(data);
}
