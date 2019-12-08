#ifndef SURV_BREAK_H
#define SURV_BREAK_H

#define SURV_MAX_BRK 10

void SurvBrk_Start(SurvivalData* data, BlockID bid);
void SurvBrk_Stop(SurvivalData* data);
void SurvBrk_Tick(SurvivalData* data, cs_uint32 delta);
#endif // SURV_BREAK_H
