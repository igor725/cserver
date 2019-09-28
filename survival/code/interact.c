#include <core.h>
#include <client.h>

#include "data.h"
#include "damage.h"
#include "interact.h"

void SurvivalInt_Start(SURVDATA* a, SURVDATA* b) {
	CLIENT ca = a->client;
	CLIENT cb = b->client;

	if(a->pvpMode && b->pvpMode) {
		SurvDamage_Hurt(b, a, SURV_DEFAULT_HIT);
		// TODO: Knockback
	} else {
		if(!a->pvpMode) Client_Chat(ca, 0, "Enable pvp mode (/pvp) first.");
	}
}
