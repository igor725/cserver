#ifndef CLIENT_H
#define CLIENT_H

#define CLIENT_OK         0
#define CLIENT_WAITCLOSE  1
#define CLIENT_AFTERCLOSE 2

#define STATE_MOTD         0
#define STATE_WLOAD        1
#define STATE_INGAME       2

typedef struct cpeData {
	bool cpeEnabled;
	short   _extCount;
	EXT*    firstExtension;
	EXT*    tailExtension;
	char*   appName;
} CPEDATA;

typedef struct playerData {
	char*   key;
	char*   name;
	int     state;
	ANGLE*  angle;
	VECTOR* position;
	void*   mapThread;
	WORLD*  currentWorld;
} PLAYERDATA;

typedef struct client {
	int         id;
	SOCKET      sock;
	int         status;
	char*       rdbuf;
	char*       wrbuf;
	ushort      bufpos;
	void*       thread;
	CPEDATA*    cpeData;
	PLAYERDATA* playerData;
} CLIENT;

bool Client_IsSupportExt(CLIENT* self, const char* packetName);
int  Client_Send(CLIENT* self, int len);
int  AcceptClients_ThreadProc(void* lpParam);
void Client_HandlePacket(CLIENT* self);
bool Client_SendMap(CLIENT* self);
bool Client_CheckAuth(CLIENT* self);
void Client_InitListen();
void AcceptClients();

CLIENT* clients[256];
void* listenThread;
#endif
