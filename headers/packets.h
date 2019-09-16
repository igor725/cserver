#ifndef PACKETS_H
#define PACKETS_H
#include "cpe.h"

#define PacketWriter_Start(client) \
char* data = client->wrbuf; \
Mutex_Lock(client->mutex); \

#define PacketWriter_End(client, size) \
Client_Send(client, size); \
Mutex_Unlock(client->mutex); \

#define PacketWriter_Stop(client) \
Mutex_Unlock(client->mutex); \
return; \

typedef bool (*packetHandler)(CLIENT*, char*);

typedef struct packet {
	const char* name;
	ushort size;
	bool haveCPEImp;
	const char* extName;
	int extVersion;
	ushort extSize;
	packetHandler handler;
	packetHandler cpeHandler;
} PACKET;

short Packet_GetSize(int id, CLIENT* client);
API void Packet_Register(int id, const char* name, ushort size, packetHandler handler);
API void Packet_RegisterCPE(int id, const char* extName, int extVersion, ushort extSize);
void Packet_RegisterDefault();
void Packet_RegisterCPEDefault();

void Packet_WriteKick(CLIENT* cl, const char* reason);
void Packet_WriteSpawn(CLIENT* client, CLIENT* other);
void Packet_WriteDespawn(CLIENT* client, CLIENT* other);
void Packet_WriteHandshake(CLIENT* cl, const char* name, const char* motd);
void Packet_WriteUpdateType(CLIENT* cl);
void Packet_WriteLvlInit(CLIENT* client);
void Packet_WriteLvlFin(CLIENT* client);
void Packet_WritePosAndOrient(CLIENT* client, CLIENT* other);
void Packet_WriteChat(CLIENT* client, MessageType type, const char* mesg);
void Packet_WriteSetBlock(CLIENT* client, ushort x, ushort y, ushort z, BlockID block);

int  ReadString(const char* data, char** dst);
void WriteString(char* data, const char* string);

bool Handler_Handshake(CLIENT* client, char* data);
bool Handler_SetBlock(CLIENT* client, char* data);
bool Handler_PosAndOrient(CLIENT* client, char* data);
bool Handler_Message(CLIENT* client, char* data);

extern PACKET* packets[256];
#endif
