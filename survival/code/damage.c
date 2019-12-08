#include <core.h>
#include <client.h>

#include "data.h"
#include "gui.h"
#include "damage.h"

void SurvDmg_Hurt(SurvivalData* target, SurvivalData* attacker, cs_uint8 damage) {
	if(damage <= 0 || target->godMode) return;
	(void)attacker;

	target->health -= min(damage, target->health);
	SurvGui_DrawHealth(target);
}

void SurvDmg_Tick(SurvivalData* data, cs_uint32 delta) {
	if(data->health < 20) {
		data->regenTimer += (cs_uint16)delta;
		if(data->regenTimer > 1) {
			data->regenTimer = 0;
			data->health += 1;
		}
	}
}
