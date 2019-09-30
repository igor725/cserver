#include <core.h>
#include <client.h>

#include "data.h"
#include "break.h"

void SurvivalBrk_Start(SURVDATA* survData, short x, short y, short z) {
	if(survData->breakStarted) return;

	survData->breakStarted = true;
	Log_Debug("Breaking (%d, %d, %d) started", x, y,z);
}

void SurvivalBrk_Stop(SURVDATA* survData) {
	if(!survData->breakStarted) return;

	survData->breakStarted = false;
	Log_Debug("Breaking stopped");
}
