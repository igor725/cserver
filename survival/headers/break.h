#ifndef SURV_BREAK_H
#define SURV_BREAK_H

#define SURV_MAX_BRK 10

void SurvivalBrk_Start(SURVDATA survData, short x, short y, short z, BlockID bid);
void SurvivalBrk_Stop(SURVDATA survData);
void SurvivalBrk_Tick(SURVDATA survData);
#endif
