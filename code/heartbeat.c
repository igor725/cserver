#include "core.h"
#include "server.h"
#include "svmath.h"
#include "heartbeat.h"

uint16_t Heartbeat_Delay = 5000, Timer = 0;
const char* Heartbeat_URL = NULL;

static void NewSecret(void) {
	RNGState secrnd;
	Random_Seed(&secrnd, (int)Time_GetMSec());
	for(int i = 0; i < 16; i++) {
		int min, max;
		switch(Random_Range(&secrnd, 0, 3)) {
			case 0:
				min = 48;
				max = 57;
				break;
			case 1:
				min = 65;
				max = 90;
				break;
			default:
				min = 97;
				max = 122;
				break;
		}
		Heartbeat_Secret[i] = (char)Random_Range(&secrnd, min, max);
	}
}

static void DoRequest() {
	if(*Heartbeat_Secret == '\0') NewSecret();
	//TODO: Осуществлять запрос к classicube.net
}

void Heartbeat_Tick(void) {
	if(!Heartbeat_Enabled) return;

	if(Timer == 0) {
		DoRequest();
		Timer = Heartbeat_Delay;
	}

	Timer -= Server_Delta;
}
