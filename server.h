#ifndef SERVER_H
#define SERVER_H
boolean Client_IsSupportExt(CLIENT* self, const char* packetName);
void InitialWork();
void DoServerStep();

SOCKET server;
WORLD* World;
#endif
