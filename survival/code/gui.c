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
	for(int i = 0; i < hlt; i++) {
		String_Append(healthstr, 20, "\3");
	}

	Client_Chat(data->client, SURV_HEALTH_POS, healthstr);
}

void SurvGui_DrawBreakProgress(SURVDATA* data) {
	char breakstr[15] = {0};
	String_Append(breakstr, 10, "[");
	for(int i = 0; i < 10; i++) {
		if(i < data->breakProgress) {

		} else if(i == data->breakProgress) {
			String_Append(breakstr, 15, "");
		}
	}
	String_Append(breakstr, 10, "&f]");
}

void SurvGui_DrawAll(SURVDATA* data) {
	SurvGui_DrawHealth(data);
}
