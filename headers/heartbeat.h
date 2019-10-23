#ifndef HEARTBEAT_H
#define HEARTBEAT_H
VAR const char* Heartbeat_URL;

void Heartbeat_Start(uint32_t delay);
void Heartbeat_Close(void);
#endif
