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

int  Packet_GetSize(int id, CLIENT* self);
void Packet_Register(int id, const char* name, ushort size, packetHandler handler);
void Packet_RegisterCPE(int id, const char* extName, int extVersion, ushort extSize);
void Packet_RegisterDefault();

void Packet_WriteKick(CLIENT* cl, const char* reason);
void Packet_WriteHandshake(CLIENT* cl);
void Packet_WriteLvlInit(CLIENT* self);
void CPEPacket_WriteInfo(CLIENT *cl);
void CPEPacket_WriteExtEntry(CLIENT *cl, EXT* ext);

int  ReadString(char* data, char** dst);
void WriteString(char* data, const char* string);

bool Handler_Handshake(CLIENT* self, char* data);
bool Handler_SetBlock(CLIENT* self, char* data);
bool Handler_PosAndOrient(CLIENT* self, char* data);
bool Handler_Message(CLIENT* self, char* data);

bool CPEHandler_ExtInfo(CLIENT* self, char* data);
bool CPEHandler_ExtEntry(CLIENT* self, char* data);

extern EXT* firstExtension;
extern EXT* tailExtension;
extern PACKET* packets[256];
#endif
