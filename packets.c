#include <winsock2.h>
#include "core.h"
#include "world.h"
#include "client.h"
#include "server.h"
#include "packets.h"

PACKET* packets[256] = {0};
EXT* firstExtension = NULL;
EXT* tailExtension = NULL;
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
	memset(tmp, 0, sizeof(struct packet));

	tmp->name = name;
	tmp->size = size;
	tmp->handler = handler;
	packets[id] = tmp;
}

void Packet_RegisterDefault() {
	Packet_Register(0x00, "Handshake", 131, &Handler_Handshake);
	Packet_Register(0x05, "SetBlock", 9, &Handler_SetBlock);
	Packet_Register(0x08, "PosAndOrient", 10, &Handler_PosAndOrient);
	Packet_Register(0x0D, "Message", 66, &Handler_Message);
}

void Packet_RegisterCPE(uchar id, const char* name, int version, ushort size) {
	PACKET* tmp = packets[id];
	tmp->extName = name;
	tmp->extVersion = version;
	tmp->extSize = size;
}

short Packet_GetSize(uchar id, CLIENT* self) {
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

void Packet_WriteKick(CLIENT* self, const char* reason) {
	self->wrbuf[0] = 0x0E;
	WriteString(self->wrbuf + 1, reason);
	Client_Send(self, 65);
	Client_Disconnect(self);
	printf("Client %d kicked %s\n", self->sock, reason);
}

void Packet_WriteHandshake(CLIENT* self) {
	char* data = self->wrbuf;
	*data = 0x00;
	*++data = 0x07;
	WriteString(++data, "Server Name"); data += 63;
	WriteString(++data, "Server MOTD"); data += 63;
	*++data = 0x00;
	*++data = 0x02; //TODO: Отдельный пакет инициализации мира, сейчас я пиздец ленивый для этого
	Client_Send(self, 132);
}

boolean Handler_Handshake(CLIENT* self, char* data) {
	uchar protoVer = *data;
	if(protoVer != 0x07) {
		Packet_WriteKick(self, "Invalid protocol version");
		return true;
	}

	self->playerData = (PLAYERDATA*)malloc(sizeof(struct playerData));
	memset(self->playerData, 0, sizeof(struct playerData));

	ReadString(++data, &self->playerData->name); data += 63;
	ReadString(++data, &self->playerData->key); data += 63;
	boolean cpeEnabled = *++data == 0x42;
	Packet_WriteHandshake(self);

	if(cpeEnabled) {
		self->cpeData = (CPEDATA*)malloc(sizeof(struct cpeData));
		memset(self->cpeData, 0, sizeof(struct cpeData));

		CPEPacket_WriteInfo(self);
		EXT* ptr = firstExtension;
		while(ptr != NULL) {
			CPEPacket_WriteExtEntry(self, ptr);
			ptr = ptr->next;
		}
	} else {
		Client_SendMap(self);
	}

	return true;
}

boolean Handler_SetBlock(CLIENT* self, char* data) {
	return false;
}

boolean Handler_PosAndOrient(CLIENT* self, char* data) {
	return false;
}

boolean Handler_Message(CLIENT* self, char* data) {
	return false;
}

/*
	CPE
*/

void Packet_RegisterCPEDefault() {
	Packet_Register(0x10, "ExtInfo", 67, &CPEHandler_ExtInfo);
	Packet_Register(0x11, "ExtEntry", 69, &CPEHandler_ExtEntry);
}

void CPEPacket_WriteInfo(CLIENT *self) {
	char* data = self->wrbuf;
	*data = 0x10;
	WriteString(++data, SOFTWARE_NAME DELIM SOFTWARE_VERSION); data += 63;
	*(ushort*)++data = htons(extensionsCount);
	Client_Send(self, 67);
}

void CPEPacket_WriteExtEntry(CLIENT* self, EXT* ext) {
	char* data = self->wrbuf;
	*data = 0x011;
	WriteString(++data, ext->name); data += 63;
	*(uint*)++data = htonl(ext->version);
	Client_Send(self, 69);
}

boolean CPEHandler_ExtInfo(CLIENT* self, char* data) {
	if(self->cpeData == NULL) return false;

	ReadString(data, &self->cpeData->appName); data += 63;
	self->cpeData->_extCount = ntohs(*(ushort*)++data);
	return true;
}

boolean CPEHandler_ExtEntry(CLIENT* self, char* data) {
	if(self->cpeData == NULL) return false;

	EXT* tmp = (EXT*)malloc(sizeof(struct ext));
	memset(tmp, 0, sizeof(struct ext));

	ReadString(data, &tmp->name);data += 63;
	tmp->version = ntohl(*(uint*)++data);

	if(self->cpeData->tailExtension == NULL) {
		self->cpeData->firstExtension = tmp;
		self->cpeData->tailExtension = tmp;
	} else {
		self->cpeData->tailExtension->next = tmp;
		self->cpeData->tailExtension = tmp;
	}

	--self->cpeData->_extCount;
	if(self->cpeData->_extCount == 0)
		Client_SendMap(self);

	return true;
}
