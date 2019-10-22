#ifndef HEARTBEAT_H
#define HEARTBEAT_H
VAR uint32_t Heartbeat_Delay;
VAR bool Heartbeat_Enabled;
VAR const char* Heartbeat_URL;
char Heartbeat_Secret[17];

void Heartbeat_Tick(void);
#endif
