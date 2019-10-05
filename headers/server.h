#ifndef SERVER_H
#define SERVER_H
#include "world.h"
#include "config.h"

THREAD Server_AcceptThread;
VAR bool Server_Active;
VAR uint16_t Server_Delta;
VAR CFGSTORE Server_Config;
SOCKET Server_Socket;
#endif
