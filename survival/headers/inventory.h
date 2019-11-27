#ifndef SURV_INV_H
#define SURV_INV_H
#define SURV_MAX_BLOCKS 999

void SurvInv_Init(SURVDATA data);
void SurvInv_UpdateInventory(SURVDATA data);
cs_uint16 SurvInv_Get(SURVDATA data, BlockID id);
cs_uint16 SurvInv_Take(SURVDATA data, BlockID id, cs_uint16 count);
cs_uint16 SurvInv_Add(SURVDATA data, BlockID id, cs_uint16 count);
#endif // SURV_INV_H
