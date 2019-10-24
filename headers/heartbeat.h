#ifndef HEARTBEAT_H
#define HEARTBEAT_H
#include "client.h"

VAR const char* Heartbeat_URL;

bool Heartbeat_CheckKey(CLIENT client);
void Heartbeat_Start(uint32_t delay);
void Heartbeat_Close(void);
#endif
