#ifndef PACKETS_H
#define PACKETS_H
typedef void (*packetHandler)(CLIENT*, char*);

typedef struct packet {
	const char* name;
	ushort size;
	boolean haveCPEImp;
	const char* extName;
	int extVersion;
	ushort extSize;
	packetHandler handler;
	packetHandler cpeHandler;
} PACKET;

typedef struct ext {
	char* name;
	int   version;
	struct ext*  next;
} EXT;

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

void Handler_Handshake(CLIENT* self, char* data);
void Handler_SetBlock(CLIENT* self, char* data);
void Handler_PosAndOrient(CLIENT* self, char* data);
void Handler_Message(CLIENT* self, char* data);

void CPEHandler_ExtInfo(CLIENT* self, char* data);
void CPEHandler_ExtEntry(CLIENT* self, char* data);

extern EXT* firstExtenison;
extern EXT* tailExtension;
extern PACKET* packets[256];
#endif
