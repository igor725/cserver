#ifndef SURV_DATA_H
#define SURV_DATA_H
#define Command_OnlyForSurvival \
if(SurvData_Get(caller)->godMode) { \
	Command_Print("This command can't be used from god mode."); \
}

cs_uint16 SurvData_AssocType;

enum survActions {
	SURV_ACT_NONE,
	SURV_ACT_BREAK
};

typedef struct survivalData {
	Client client;
	cs_uint16 inventory[256];
	SVec lastClick;
	cs_uint8 health;
	cs_uint8 oxygen;
	cs_bool showOxygen;
	cs_bool godMode;
	cs_bool pvpMode;
	cs_bool breakStarted;
	cs_int16 breakTimer;
	cs_uint8 breakProgress;
	BlockID breakBlock;
} *SURVDATA;

void SurvData_Create(Client cl);
void SurvData_Free(Client cl);
SURVDATA SurvData_Get(Client cl);
SURVDATA SurvData_GetByID(ClientID id);
#endif
