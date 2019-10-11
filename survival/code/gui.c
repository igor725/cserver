#include <core.h>
#include <client.h>
#include <packets.h>
#include <block.h>

#include "data.h"
#include "gui.h"
#include "inventory.h"

// TODO: Исправить этот пиздец
void SurvGui_DrawHealth(SURVDATA data) {
	char healthstr[20] = {0};
	uint8_t hltf = data->health / 2;
	uint8_t empty = 10 - hltf;

	String_Append(healthstr, 20, "&c");
	for(int i = 0; i < hltf; i++) {
		String_Append(healthstr, 20, "\3");
	}
	if(data->health % 2) {
		String_Append(healthstr, 20, "&4\3");
		empty -= 1;
	}
	if(empty > 0) {
		String_Append(healthstr, 20, "&8");
		for(int i = 0; i < empty; i++) {
			String_Append(healthstr, 20, "\3");
		}
	}

	Client_Chat(data->client, CPE_STATUS1, healthstr);
}

void SurvGui_DrawBreakProgress(SURVDATA data) {
	char breakstr[19] = {0};

	if(data->breakStarted) {
		String_Append(breakstr, 10, "[&a");
		for(int i = 0; i < 10; i++) {
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
	CLIENT client = data->client;

	if(id > BLOCK_AIR) {
		const char* bn = Block_GetName(id);
		uint16_t bc = SurvInv_Get(data, id);
		String_FormatBuf(blockinfo, 64, "%s (%d)", bn, bc);
	}

	Client_Chat(client, CPE_BRIGHT1, blockinfo);
}

void SurvGui_DrawAll(SURVDATA data) {
	SurvGui_DrawHealth(data);
}
