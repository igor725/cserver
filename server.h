#ifndef SERVER_H
#define SERVER_H
#include "winsock2.h"
#include "world.h"

bool Server_InitialWork();
void Server_DoStep();

WORLD* worlds[128];
bool serverActive;
SOCKET server;
#endif
