#ifndef PACKETS_H
#define PACKETS_H

#define MODE_DESTROY 0x00
#define MODE_PLACE   0x01

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

int  Packet_GetSize(int id, CLIENT* client);
void Packet_Register(int id, const char* name, ushort size, packetHandler handler);
void Packet_RegisterCPE(int id, const char* extName, int extVersion, ushort extSize);
void Packet_RegisterDefault();
void Packet_RegisterCPEDefault();

void Packet_WriteKick(CLIENT* cl, const char* reason);
void Packet_WriteSpawn(CLIENT* client, CLIENT* other);
void Packet_WriteHandshake(CLIENT* cl);
void Packet_WriteLvlInit(CLIENT* client);
void Packet_WriteLvlFin(CLIENT* client);
void Packet_WriteChat(CLIENT* client, int type, char* mesg);

void CPEPacket_WriteInfo(CLIENT *cl);
void CPEPacket_WriteExtEntry(CLIENT *cl, EXT* ext);

int  ReadString(char* data, char** dst);
void WriteString(char* data, const char* string);

bool Handler_Handshake(CLIENT* client, char* data);
bool Handler_SetBlock(CLIENT* client, char* data);
bool Handler_PosAndOrient(CLIENT* client, char* data);
bool Handler_Message(CLIENT* client, char* data);

bool CPEHandler_ExtInfo(CLIENT* client, char* data);
bool CPEHandler_ExtEntry(CLIENT* client, char* data);

extern EXT* firstExtension;
extern EXT* tailExtension;
extern PACKET* packets[256];
#endif
