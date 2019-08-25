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

static int ReadString(char* data, char** dst) {
	int end = 63;
	while(data[end] == ' ') --end;
	++end;

	char* str = (char*)malloc(end + 1);
	memcpy(str, data, end);
	str[end] = 0;
	dst[0] = str;
	return end;
}

static void WriteString(char* data, const char* string) {
	int size = min(strlen(string), 64);
	memcpy(data, string, size);

	if(size == 64) return;
	memset(data + size, ' ', 64 - size);
}

static char* WriteShortVec(char* data, SVECTOR* vec) {
	*(ushort*)++data = htons(vec->x);++data;
	*(ushort*)++data = htons(vec->y);++data;
	*(ushort*)++data = htons(vec->z);++data;
	return data;
}

static void ReadClPos(CLIENT* self, char* data) {
	VECTOR vec = self->playerData->position;
	ANGLE ang = self->playerData->angle;

	vec.x = (float)ntohs((*(short*)data)) / 32;++data;
	vec.y = (float)ntohs((*(short*)++data)) / 32;++data;
	vec.z = (float)ntohs((*(short*)++data)) / 32;++data;
	ang.yaw = (((float)(uchar)*++data) / 256) * 360;
	ang.pitch = (((float)(uchar)*++data) / 256) * 360;
}

#define SWAP(num) ((num >> 8) | (num << 8))
static char* WriteClPos(char* data, CLIENT* self, bool stand) {
	VECTOR vec = self->playerData->position;
	ANGLE ang = self->playerData->angle;
	*(ushort*)data = htons(vec.x * 32);++data;
	*(ushort*)++data = htons(vec.y * 32 + (stand ? 51 : 0));++data;
	*(ushort*)++data = htons(vec.z * 32);++data;
	*(uchar*)++data = ((ang.yaw / 360) * 256);
	*(uchar*)++data = ((ang.pitch / 360) * 256);
	return data;
}

void Packet_Register(int id, const char* name, ushort size, packetHandler handler) {
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

void Packet_RegisterCPE(int id, const char* name, int version, ushort size) {
	PACKET* tmp = packets[id];
	tmp->extName = name;
	tmp->extVersion = version;
	tmp->extSize = size;
}

int Packet_GetSize(int id, CLIENT* self) {
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
	if(self->status != CLIENT_OK)
		return;

	self->wrbuf[0] = 0x0E;
	WriteString(self->wrbuf + 1, reason);
	Client_Send(self, 65);
	Client_Disconnect(self);
	printf("Client %d kicked %s\n", self->sock, reason);
}

void Packet_WriteLvlInit(CLIENT* self) {
	self->wrbuf[0] = 0x02;
	self->playerData->state = STATE_MOTD;
	Client_Send(self, 1);
}

void Packet_WriteLvlFin(CLIENT* self) {
	WORLD* world = self->playerData->currentWorld;
	char* data = self->wrbuf;
	*data = 0x04;
	WriteShortVec(data, (SVECTOR*)world->dimensions);
	Client_Send(self, 7);
}

void Packet_WriteHandshake(CLIENT* self) {
	char* data = self->wrbuf;
	*data = 0x00;
	*++data = 0x07;
	WriteString(++data, "Server Name"); data += 63;
	WriteString(++data, "Server MOTD"); data += 63;
	*++data = 0x00;
	Client_Send(self, 131);
}

void Packet_WriteSpawn(CLIENT* self, CLIENT* other) {
	char* data = self->wrbuf;
	*data = 0x07;
	*++data = self == other ? 0xFF : other->id;
	WriteString(++data, other->playerData->name); data += 63;
	WriteClPos(++data, other, true);
	Client_Send(self, 74);
}

void Packet_WritePosAndOrient(CLIENT* self, CLIENT* other) {
	char* data = self->wrbuf;
	*data = 0x08;
	//TODO: Дописать пакет перемещения
}

bool Handler_Handshake(CLIENT* self, char* data) {
	uchar protoVer = *data;
	if(protoVer != 0x07) {
		Packet_WriteKick(self, "Invalid protocol version");
		return true;
	}

	self->playerData = (PLAYERDATA*)malloc(sizeof(struct playerData));
	memset(self->playerData, 0, sizeof(struct playerData));
	self->playerData->currentWorld = worlds[0];
	Client_SetPos(self, &worlds[0]->spawnVec, &worlds[0]->spawnAng);
	ReadString(++data, &self->playerData->name); data += 63;
	ReadString(++data, &self->playerData->key); data += 63;
	bool cpeEnabled = *++data == 0x42; // Temporarily

	if(Client_CheckAuth(self))
		Packet_WriteHandshake(self);
	else {
		Packet_WriteKick(self, "Auth failed");
		return true;
	}

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

bool Handler_SetBlock(CLIENT* self, char* data) {
	WORLD* world = self->playerData->currentWorld;
	if(world == NULL) return false;

	ushort x = ntohs(*(ushort*)data); data += 2;
	ushort y = ntohs(*(ushort*)data); data += 2;
	ushort z = ntohs(*(ushort*)data); data += 2;
	uchar mode = *(uchar*)data; ++data;
	int block = (int)*(uchar*)data; ++data;

	switch(mode) {
		case MODE_PLACE:
			if(!Block_IsValid(block)) {
				Packet_WriteKick(self, "Invalid block ID");
				return false;
			}
			if(Event_OnBlockPalce(self, x, y, z, block))
				World_SetBlock(world, x, y, z, block);
			break;
		case MODE_DESTROY:
			if(Event_OnBlockPalce(self, x, y, z, 0))
				World_SetBlock(world, x, y, z, 0);
			break;
	}

	return true;
}

bool Handler_PosAndOrient(CLIENT* self, char* data) {
	int id = *data;
	ReadClPos(self, ++data);
	//TODO: Рассылка игрокам позиций других игроков
	return true;
}

bool Handler_Message(CLIENT* self, char* data) {
	char* message;
	uchar unused = *data;
	int len = ReadString(++data, &message);
	Event_OnMessage(self, message, len);
	free(message);
	return true;
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

bool CPEHandler_ExtInfo(CLIENT* self, char* data) {
	if(self->cpeData == NULL) return false;

	ReadString(data, &self->cpeData->appName); data += 63;
	self->cpeData->_extCount = ntohs(*(ushort*)++data);
	return true;
}

bool CPEHandler_ExtEntry(CLIENT* self, char* data) {
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
