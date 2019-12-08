#ifndef SURV_INV_H
#define SURV_INV_H
#define SURV_MAX_BLOCKS 999

void SurvInv_Init(SurvivalData* data);
void SurvInv_UpdateInventory(SurvivalData* data);
cs_uint16 SurvInv_Get(SurvivalData* data, BlockID id);
cs_uint16 SurvInv_Take(SurvivalData* data, BlockID id, cs_uint16 count);
cs_uint16 SurvInv_Add(SurvivalData* data, BlockID id, cs_uint16 count);
#endif // SURV_INV_H
