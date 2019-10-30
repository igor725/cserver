#ifndef SURV_DATA_H
#define SURV_DATA_H
#define Command_OnlyForSurvival \
if(SurvData_Get(caller)->godMode) { \
	Command_Print("This command can't be used from god mode."); \
}

enum survActions {
	SURV_ACT_NONE,
	SURV_ACT_BREAK
};

typedef struct survivalData {
	Client client;
	uint16_t inventory[256];
	SVec lastClick;
	uint8_t health;
	uint8_t oxygen;
	bool showOxygen;
	bool godMode;
	bool pvpMode;
	bool breakStarted;
	int16_t breakTimer;
	uint8_t breakProgress;
	BlockID breakBlock;
} *SURVDATA;

void SurvData_Create(Client cl);
void SurvData_Free(Client cl);
SURVDATA SurvData_Get(Client cl);
SURVDATA SurvData_GetByID(ClientID id);
#endif
