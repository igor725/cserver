#ifndef SURV_DATA_H
#define SURV_DATA_H
enum survActions {
	SURV_ACT_NONE,
	SURV_ACT_BREAK
};

typedef struct survivalData {
	CLIENT client;
	BlockID inventory[256];
	float health;
	float oxygen;
	bool showOxygen;
	bool godMode;
	bool pvpMode;
	short lastclick[3];
	bool breakStarted;
	int breakTimer;
	uint8_t breakProgress;
	BlockID breakBlock;
} *SURVDATA;

void SurvData_Create(CLIENT cl);
void SurvData_Free(CLIENT cl);
SURVDATA SurvData_Get(CLIENT cl);
SURVDATA SurvData_GetByID(ClientID id);
#endif
