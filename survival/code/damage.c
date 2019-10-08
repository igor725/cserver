#include <core.h>
#include <client.h>

#include "data.h"
#include "gui.h"
#include "damage.h"

void SurvDamage_Hurt(SURVDATA target, SURVDATA attacker, float damage) {
	if(damage <= 0 || target->godMode) return;

	target->health -= min(damage, target->health);
	SurvGui_DrawHealth(target);
}

void SurvDamage_Tick(void) {

}
