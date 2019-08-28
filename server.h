#ifndef SERVER_H
#define SERVER_H
#include "core.h"
#include "world.h"

bool Server_InitialWork();
void Server_DoStep();

WORLD* worlds[128];
THREAD acceptThread;
bool serverActive;
SOCKET server;
#endif
