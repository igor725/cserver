#ifndef CLIENT_H
#define CLIENT_H

#define CLIENT_OK         0
#define CLIENT_WAITCLOSE  1
#define CLIENT_AFTERCLOSE 2

typedef struct cpeData {
	boolean cpeEnabled;
	short   _extCount;
	EXT*    firstExtension;
	EXT*    tailExtension;
	char*   appName;
} CPEDATA;

typedef struct playerData {
	char*   name;
	char*   key;
	ANGLE*  angle;
	VECTOR* position;
	WORLD*  currentWorld;
} PLAYERDATA;

typedef struct client {
	int         id;
	SOCKET      sock;
	int         state;
	char*       rdbuf;
	char*       wrbuf;
	ushort      bufpos;
	void*       thread;
	CPEDATA*    cpeData;
	PLAYERDATA* playerData;
} CLIENT;

boolean Client_IsSupportExt(CLIENT* self, const char* packetName);
int Client_Send(CLIENT* self, uint len);
int AcceptClients_ThreadProc(void* lpParam);
void Client_HandlePacket(CLIENT* self);
boolean Client_SendMap(CLIENT* self);
void Client_InitListen();
void AcceptClients();

CLIENT* clients[256];
void* listenThread;
#endif
