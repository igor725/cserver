#ifndef HEARTBEAT_H
#define HEARTBEAT_H
#include "client.h"

bool Heartbeat_CheckKey(Client client);
void Heartbeat_Start(uint32_t delay);
void Heartbeat_Close(void);
#endif
