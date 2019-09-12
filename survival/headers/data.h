#ifndef SURV_DATA_H
#define SURV_DATA_H
typedef struct survivalData {
	CLIENT* client;
	BlockID inventory[256];
	float health;
	float oxygen;
} SURVDATA;

void SurvData_Create(CLIENT* cl);
SURVDATA* SurvData_Get(CLIENT* cl);
#endif
