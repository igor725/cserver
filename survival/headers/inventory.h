#ifndef SURV_INV_H
#define SURV_INV_H
#define SURV_MAX_BLOCKS 999

void SurvInv_Init(SURVDATA data);
void SurvInv_UpdateInventory(SURVDATA data);
uint16_t SurvInv_Get(SURVDATA data, BlockID id);
uint16_t SurvInv_Take(SURVDATA data, BlockID id, uint16_t count);
uint16_t SurvInv_Add(SURVDATA data, BlockID id, uint16_t count);
#endif
