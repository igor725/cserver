#ifndef PACKETS_H
#define PACKETS_H
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

short Packet_GetSize(uchar id, CLIENT* self);
void Packet_Register(uchar id, const char* name, ushort size, packetHandler handler);
void Packet_RegisterCPE(uchar id, const char* extName, int extVersion, ushort extSize);
void Packet_WriteKick(CLIENT* cl, const char* reason);
void Packet_WriteHandshake(CLIENT* cl);
void CPEPacket_WriteInfo(CLIENT *cl);
void CPEPacket_WriteExtEntry(CLIENT *cl, EXT* ext);
void Packet_RegisterDefault();
uchar ReadString(char* data, char** dst);
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
