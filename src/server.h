#ifndef SERVER_H
#define SERVER_H
#include "core.h"
#include "types/platform.h"
#include "types/config.h"

#define CFG_SERVERIP_KEY "server-ip"
#define CFG_SERVERPORT_KEY "server-port"
#define CFG_SERVERNAME_KEY "server-name"
#define CFG_SERVERMOTD_KEY "server-motd"
#define CFG_LOGLEVEL_KEY "log-level"
#define CFG_SANITIZE_KEY "sanitize-names"
#define CFG_LOCALOP_KEY "always-local-op"
#define CFG_MAXPLAYERS_KEY "max-players"
#define CFG_CONN_KEY "max-connections-per-ip"
#define CFG_WORLDS_KEY "worlds-list"

VAR cs_bool Server_Active, Server_Ready;
VAR CStore *Server_Config;
VAR cs_uint64 Server_StartTime;
VAR cs_str Server_Version;
extern Socket Server_Socket;

cs_bool Server_Init(void);
void Server_DoStep(cs_int32 delta);
void Server_StartLoop(void);
void Server_Cleanup(void);

API cs_str Server_GetAppName(void);
#endif // SERVER_H
