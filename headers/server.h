#ifndef SERVER_H
#define SERVER_H
#include "world.h"
#include "config.h"

#define MAINCFG "server.cfg"

bool Server_InitialWork();
void Server_DoStep();
THREAD acceptThread;
bool serverActive;
VAR CFGSTORE* mainCfg;
SOCKET server;
#endif
