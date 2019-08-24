#ifndef SERVER_H
#define SERVER_H
bool Server_InitialWork();
void Server_DoStep();

bool serverActive;
SOCKET server;
WORLD* World;
#endif
