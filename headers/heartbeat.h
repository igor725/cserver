#ifndef HEARTBEAT_H
#define HEARTBEAT_H
VAR const char* Heartbeat_URL;
char Heartbeat_Secret[17];

void Heartbeat_Start(uint32_t delay);
void Heartbeat_Close(void);
#endif
