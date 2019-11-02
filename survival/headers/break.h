#ifndef SURV_BREAK_H
#define SURV_BREAK_H

#define SURV_MAX_BRK 10

void SurvBrk_Start(SURVDATA data, BlockID bid);
void SurvBrk_Stop(SURVDATA data);
void SurvBrk_Tick(SURVDATA data, uint32_t delta);
#endif
