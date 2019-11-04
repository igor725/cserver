#ifndef PACKETS_H
#define PACKETS_H
#include "cpe.h"

#define ValidateClientState(client, st, ret) \
if(!client->playerData || client->playerData->state != st) \
	return ret; \

#define PacketWriter_Start(client) \
if(client->closed) return; \
char* data = client->wrbuf; \
Mutex_Lock(client->mutex); \

#define PacketWriter_End(client, size) \
Client_Send(client, size); \
Mutex_Unlock(client->mutex); \

#define PacketWriter_Stop(client) \
Mutex_Unlock(client->mutex); \
return; \

typedef bool(*packetHandler)(Client, const char*);

typedef struct packet {
	cs_uint16 size;
	bool haveCPEImp;
	cs_uint32 extCRC32;
	cs_int32 extVersion;
	cs_uint16 extSize;
	packetHandler handler;
	packetHandler cpeHandler;
} *Packet;

Packet Packet_Get(cs_int32 id);
API void Packet_Register(cs_int32 id, cs_uint16 size, packetHandler handler);
API void Packet_RegisterCPE(cs_int32 id, cs_uint32 extCRC32, cs_int32 extVersion, cs_uint16 extSize, packetHandler handler);

API cs_uint8 Proto_ReadString(const char** data, const char** dst);
API cs_uint8 Proto_ReadStringNoAlloc(const char** data, char* dst);
API void Proto_ReadSVec(const char** dataptr, SVec* vec);
API void Proto_ReadAng(const char** dataptr, Ang* ang);
API void Proto_ReadFlSVec(const char** dataptr, Vec* vec);
API void Proto_ReadFlVec(const char** dataptr, Vec* vec);
API bool Proto_ReadClientPos(Client client, const char* data);

API void Proto_WriteString(char** dataptr, const char* string);
API void Proto_WriteFlVec(char** dataptr, const Vec* vec);
API void Proto_WriteFlSVec(char** dataptr, const Vec* vec);
API void Proto_WriteSVec(char** dataptr, const SVec* vec);
API void Proto_WriteAng(char** dataptr, const Ang* ang);
API void Proto_WriteColor3(char** dataptr, const Color3* color);
API void Proto_WriteColor4(char** dataptr, const Color4* color);
API cs_uint32 Proto_WriteClientPos(char* data, Client client, bool extended);

void Packet_RegisterDefault(void);
void Packet_RegisterCPEDefault(void);

void Packet_WriteHandshake(Client client, const char* name, const char* motd);
void Packet_WriteLvlInit(Client client);
void Packet_WriteLvlFin(Client client, SVec* dims);
void Packet_WriteSetBlock(Client client, SVec* pos, BlockID block);
void Packet_WriteSpawn(Client client, Client other);
void Packet_WritePosAndOrient(Client client, Client other);
void Packet_WriteDespawn(Client client, Client other);
void Packet_WriteChat(Client client, MessageType type, const char* mesg);
void Packet_WriteKick(Client client, const char* reason);

bool Handler_Handshake(Client client, const char* data);
bool Handler_SetBlock(Client client, const char* data);
bool Handler_PosAndOrient(Client client, const char* data);
bool Handler_Message(Client client, const char* data);
#endif
