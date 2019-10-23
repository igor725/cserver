#ifndef HEARTBEAT_H
#define HEARTBEAT_H
VAR uint32_t Heartbeat_Delay;
VAR bool Heartbeat_Enabled;
VAR const char* Heartbeat_URL;

void Heartbeat_Tick(void);
#endif
