#ifndef CLIENT_H
#define CLIENT_H

#define CLIENT_OK         0
#define CLIENT_WAITCLOSE  1
#define CLIENT_AFTERCLOSE 2

typedef struct client {
	int     id;
	int     state;
	char*   name;
	char*   key;
	SOCKET  sock;
	char*   rdbuf;
	char*   wrbuf;
	ushort  rdbufpos;
	ushort  wrbufpos;
	void*   thread;
	boolean cpeEnabled;
} CLIENT;

boolean Client_IsSupportExt(CLIENT* self, const char* packetName);
int AcceptClients_ThreadProc(void* lpParam);
void Client_HandlePacket(CLIENT* self);
void Client_InitListen();
void AcceptClients();

CLIENT* clients[256];
void* listenThread;
#endif
