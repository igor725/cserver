#include <core.h>
#include <client.h>
#include <packets.h>

#include "data.h"
#include "gui.h"

void SurvGui_DrawHealth(SURVDATA* data) {
	char healthstr[20] = {0};
	int hlt = (int)data->health;
	float f = data->health - (float)hlt;

	String_Append(healthstr, 20, "&c");
	for(int i = 0; i < SURV_MAX_HEALTH; i++) {
		if (hlt == i)
			String_Append(healthstr, 20, "&4");
		else if(hlt + 1 == i)
			String_Append(healthstr, 20, "&8");

		String_Append(healthstr, 20, "\3");
	}

	Client_Chat(data->client, SURV_HEALTH_POS, healthstr);
}

void SurvGui_DrawBreakProgress(SURVDATA* data) {
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

void SurvGui_DrawAll(SURVDATA* data) {
	SurvGui_DrawHealth(data);
}
