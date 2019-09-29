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
	bool breakStarted;
	int breakProgress;
} SURVDATA;

void SurvData_Create(CLIENT cl);
SURVDATA* SurvData_Get(CLIENT cl);
#endif
