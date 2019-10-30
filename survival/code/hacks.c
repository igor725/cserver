#include <core.h>
#include <client.h>

#include "data.h"
#include "hacks.h"

void SurvHacks_Update(SURVDATA data) {
	Client client = data->client;
	Hacks hacks = client->cpeData->hacks;

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
