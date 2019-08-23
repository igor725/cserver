#include <winsock2.h>
#include <windows.h>
#include "core.h"
#include "client.h"
#include "server.h"
#include "packets.h"

PACKET* packets[256] = {0};
EXT* firstExtenison = NULL;
EXT* tailExtenison = NULL;
int extensionsCount = 0;

uchar ReadString(char* data, char** dst) {
	uchar end = 63;
	while(data[end] == ' ') --end;
	++end;

	char* str = (char*)malloc(end + 1);
	memcpy(str, data, end);
	str[end] = 0;
	dst[0] = str;
	return end;
}

void WriteString(char* data, const char* string) {
	int size = min(strlen(string), 64);
	memcpy(data, string, size);

	if(size == 64) return;
	memset(data + size, ' ', 64 - size);
}

void Packet_Register(uchar id, const char* name, ushort size, packetHandler handler) {
	PACKET* tmp = (PACKET*)malloc(sizeof(struct packet));
	tmp->name = name;
	tmp->size = size;
	tmp->handler = handler;
	packets[id] = tmp;
}

void Packet_RegisterDefault() {
	Packet_Register(0x00, "Handshake", 131, &Handler_Handshake);
}

void Packet_RegisterCPE(uchar id, const char* extName, int extVersion, ushort extSize) {
	PACKET* tmp = packets[id];
	tmp->extName = extName;
	tmp->extVersion = extVersion;
	tmp->extSize = extSize;
}

ushort Packet_GetSize(uchar id, CLIENT* self) {
	PACKET* packet = packets[id];
	if(packet == NULL)
		return -1;

	if(packet->haveCPEImp)
		return Client_IsSupportExt(self, packet->extName) ? packet->extSize : packet->size;
	else
		return packet->size;
}

/*
	VANILLA
*/

void Packet_WriteKick(CLIENT* cl, const char* reason) {
	cl->wrbuf[0] = 0x0E;
	WriteString(cl->wrbuf + 1, reason);
	send(cl->sock, cl->wrbuf, 65, 0);
	cl->state = CLIENT_WAITCLOSE;
	shutdown(cl->sock, SD_RECEIVE);
}

void Packet_WriteHandshake(CLIENT* cl) {
	char* data = cl->wrbuf;
	*data = 0x00;
	*++data = 0x07;
	WriteString(++data, "Server Name"); data += 63;
	WriteString(++data, "Server MOTD"); data += 63;
	*++data = 0x00;
	*++data = 0x02; //TODO: Отдельный пакет инициализации мира, сейчас я пиздец ленивый для этого
	send(cl->sock, cl->wrbuf, 132, 0);
}

void Handler_Handshake(CLIENT* self, char* data) {
	uchar protoVer = *++data;
	if(protoVer != 0x07) {
		Packet_WriteKick(self, "Invalid protocol version");
		return;
	}
	ReadString(++data, &self->name); data += 63;
	ReadString(++data, &self->key); data += 63;
	self->cpeEnabled = *++data == 0x42;
	Packet_WriteHandshake(self);
	if(self->cpeEnabled){
		CPEPacket_WriteInfo(self);
		EXT* ptr = firstExtenison;
		while(ptr != NULL) {
			CPEPacket_WriteExtEntry(self, ptr);
			ptr = ptr->next;
		}
	}
}

/*
	CPE
*/

void CPEPacket_WriteInfo(CLIENT *cl) {
	char* data = cl->wrbuf;
	*data = 0x10;
	WriteString(++data, SOFTWARE_NAME DELIM SOFTWARE_VERSION); data += 63;
	*(ushort*)++data = htons(extensionsCount);
	send(cl->sock, cl->wrbuf, 67, 0);
}

void CPEPacket_WriteExtEntry(CLIENT *cl, EXT* ext) {
	char* data = cl->wrbuf;
	*data = 0x011;
	WriteString(++data, ext->name); data += 63;
	*(uint*)++data = htonl(ext->version);
}
