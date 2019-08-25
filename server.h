#ifndef SERVER_H
#define SERVER_H
bool Server_InitialWork();
void Server_DoStep();

WORLD* worlds[128];
bool serverActive;
SOCKET server;
#endif
