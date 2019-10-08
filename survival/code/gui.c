#include <core.h>
#include <client.h>
#include <packets.h>

#include "data.h"
#include "gui.h"

// TODO: Исправить этот пиздец
void SurvGui_DrawHealth(SURVDATA data) {
	char healthstr[20] = {0};
	int health = (int)data->health;
	float fr = data->health - (float)health;
	int empty = SURV_MAX_HEALTH - health - (fr > 0 ? 1 : 0);

	String_Append(healthstr, 20, "&c");
	for(int i = 0; i < health; i++) {
		String_Append(healthstr, 20, "\3");
	}
	if(fr > 0) {
		String_Append(healthstr, 20, "&4\3");
	}
	if(empty > 0) {
		String_Append(healthstr, 20, "&8");
		for(int i = 0; i < empty; i++) {
			String_Append(healthstr, 20, "\3");
		}
	}

	Client_Chat(data->client, SURV_HEALTH_POS, healthstr);
}

void SurvGui_DrawBreakProgress(SURVDATA data) {
	char breakstr[19] = {0};
	
	String_Append(breakstr, 10, "[&a");
	for(int i = 0; i < 10; i++) {
		if(i == data->breakProgress)
			String_Append(breakstr, 19, "&8");

		String_Append(breakstr, 19, "|");
	}
	String_Append(breakstr, 19, "&f]");

	Client_Chat(data->client, SURV_BREAK_POS, breakstr);
}

void SurvGui_DrawAll(SURVDATA data) {
	SurvGui_DrawHealth(data);
}
