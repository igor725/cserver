#ifndef SURV_DATA_H
#define SURV_DATA_H
cs_uint16 SurvData_AssocType;

typedef struct {
	Client* client;
	cs_uint16 inventory[256];
	SVec lastClick;
	cs_uint8 health;
	cs_uint8 oxygen;
	cs_bool showOxygen;
	cs_bool godMode;
	cs_bool pvpMode;
	cs_uint16 regenTimer;
	cs_bool breakStarted;
	cs_uint16 breakTimer;
	cs_uint8 breakProgress;
	BlockID breakBlock;
} SurvivalData;

void SurvData_Create(Client* client);
void SurvData_Free(Client* client);
SurvivalData* SurvData_Get(Client* client);
SurvivalData* SurvData_GetByID(ClientID id);
#endif // SURV_DATA_H
