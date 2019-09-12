#include <core.h>
#include <packets.h>

#include "gui.h"

void SurvGui_DrawHealth(SURVDATA* data) {
	char healthstr[10];
	
	Packet_WriteChat(data->client, SURV_HEALTH_POS, healthstr);
}
