#ifndef SERVER_H
#define SERVER_H
#include "world.h"
#include "config.h"

bool Server_InitialWork();
void Server_DoStep();
THREAD Server_AcceptThread;
VAR bool Server_Active;
VAR CFGSTORE* Server_Config;
SOCKET Server_Socket;
#endif
