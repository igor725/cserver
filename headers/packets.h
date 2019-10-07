#ifndef PACKETS_H
#define PACKETS_H
#include "cpe.h"

#define ValidateClientState(client, st, ret) \
if(!client->playerData || client->playerData->state != st) \
	return ret; \

#define PacketWriter_Start(client) \
char* data = client->wrbuf; \
Mutex_Lock(client->mutex); \

#define PacketWriter_End(client, size) \
Client_Send(client, size); \
Mutex_Unlock(client->mutex); \

#define PacketWriter_Stop(client) \
Mutex_Unlock(client->mutex); \
return; \

typedef bool (*packetHandler)(CLIENT, char*);

typedef struct packet {
	const char* name;
	uint16_t size;
	bool haveCPEImp;
	const char* extName;
	int extVersion;
	uint16_t extSize;
	packetHandler handler;
	packetHandler cpeHandler;
} *PACKET;

PACKET Packet_Get(int id);
API void Packet_Register(int id, const char* name, uint16_t size, packetHandler handler);
API void Packet_RegisterCPE(int id, const char* extName, int extVersion, uint16_t extSize, packetHandler handler);
void Packet_RegisterDefault(void);
void Packet_RegisterCPEDefault(void);

void Packet_WriteKick(CLIENT cl, const char* reason);
void Packet_WriteSpawn(CLIENT client, CLIENT other);
void Packet_WriteDespawn(CLIENT client, CLIENT other);
void Packet_WriteHandshake(CLIENT cl, const char* name, const char* motd);
void Packet_WriteUpdateType(CLIENT cl);
void Packet_WriteLvlInit(CLIENT client);
void Packet_WriteLvlFin(CLIENT client);
void Packet_WritePosAndOrient(CLIENT client, CLIENT other);
void Packet_WriteChat(CLIENT client, MessageType type, const char* mesg);
void Packet_WriteSetBlock(CLIENT client, uint16_t x, uint16_t y, uint16_t z, BlockID block);

uint8_t ReadNetString(const char* data, const char** dst);
void WriteNetString(char* data, const char* string);
void ReadClPos(CLIENT client, char* data, bool extended);
uint32_t WriteClPos(char* data, CLIENT client, bool stand, bool extended);

bool Handler_Handshake(CLIENT client, char* data);
bool Handler_SetBlock(CLIENT client, char* data);
bool Handler_PosAndOrient(CLIENT client, char* data);
bool Handler_Message(CLIENT client, char* data);
#endif
