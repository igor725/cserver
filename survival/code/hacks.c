#include <core.h>
#include <client.h>

#include "data.h"
#include "hacks.h"

void SurvHacks_Update(SURVDATA data) {
	CLIENT client = data->client;
	HACKS hacks = client->cpeData->hacks;

	hacks->tpv = true;
	hacks->spawnControl = false;
	hacks->jumpHeight = -1;

	if(data->godMode) {
		hacks->flying = true;
		hacks->noclip = true;
		hacks->speeding = true;
	} else {
		hacks->flying = false;
		hacks->noclip = false;
		hacks->speeding = false;
	}

	Client_SetHacks(client);
}
