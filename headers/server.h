#ifndef SERVER_H
#define SERVER_H
#include "world.h"

bool Server_InitialWork();
void Server_DoStep();
THREAD acceptThread;
bool serverActive;
SOCKET server;
#endif
