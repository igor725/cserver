#ifndef SURV_DATA_H
#define SURV_DATA_H
#define Command_OnlyForSurvival(a, b) \
if((b)->godMode) { \
	Command_Print((a), "This command can't be used from god mode."); \
}

cs_uint16 SurvData_AssocType;

typedef struct survivalData {
	Client client;
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
} *SURVDATA;

void SurvData_Create(Client cl);
void SurvData_Free(Client cl);
SURVDATA SurvData_Get(Client cl);
SURVDATA SurvData_GetByID(ClientID id);
#endif // SURV_DATA_H
