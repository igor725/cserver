#include "core.h"
#include "block.h"
#include "world.h"
#include "client.h"
#include "event.h"
#include "server.h"
#include "packets.h"
#include "command.h"
#include "cpe.h"

PACKET* packets[256] = {0};

int ReadString(char* data, char** dst) {
	int end = 63;
	while(data[end] == ' ') --end;
	++end;

	char* str = calloc(end + 1, 1);
	memcpy(str, data, end);
	str[end] = 0;
	dst[0] = str;
	return end;
}

void WriteString(char* data, const char* string) {
	int size = min(strlen(string), 64);
	memset(data + size, 0, 64);
	memcpy(data, string, size);
}

char* WriteShortVec(char* data, SVECTOR* vec) {
	*(ushort*)++data = htons(vec->x);++data;
	*(ushort*)++data = htons(vec->y);++data;
	*(ushort*)++data = htons(vec->z);++data;
	return data;
}

void ReadClPos(CLIENT* client, char* data) {
	VECTOR* vec = client->playerData->position;
	ANGLE* ang = client->playerData->angle;

	vec->x = (float)ntohs((*(short*)data)) / 32;++data;
	vec->y = (float)ntohs((*(short*)++data)) / 32;++data;
	vec->z = (float)ntohs((*(short*)++data)) / 32;++data;
	ang->yaw = (((float)(uchar)*++data) / 256) * 360;
	ang->pitch = (((float)(uchar)*++data) / 256) * 360;
}

char* WriteClPos(char* data, CLIENT* client, bool stand) {
	VECTOR* vec = client->playerData->position;
	ANGLE* ang = client->playerData->angle;

	*(ushort*)data = htons(vec->x * 32);++data;
	*(ushort*)++data = htons(vec->y * 32 + (stand ? 51 : 0));++data;
	*(ushort*)++data = htons(vec->z * 32);++data;
	*(uchar*)++data = ((ang->yaw / 360) * 256);
	*(uchar*)++data = ((ang->pitch / 360) * 256);

	return data;
}

void Packet_Register(int id, const char* name, ushort size, packetHandler handler) {
	PACKET* tmp = calloc(1, sizeof(struct packet));

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

int Packet_GetSize(int id, CLIENT* client) {
	PACKET* packet = packets[id];
	if(!packet)
		return -1;

	if(packet->haveCPEImp)
		return Client_IsSupportExt(client, packet->extName) ? packet->extSize : packet->size;
	else
		return packet->size;
}

/*
	VANILLA
*/

void Packet_WriteKick(CLIENT* client, const char* reason) {
	if(client->status != CLIENT_OK)
		return;

	client->wrbuf[0] = 0x0E;
	WriteString(client->wrbuf + 1, reason);
	Client_Send(client, 65);
}

void Packet_WriteLvlInit(CLIENT* client) {
	client->wrbuf[0] = 0x02;
	Client_Send(client, 1);
}

void Packet_WriteLvlFin(CLIENT* client) {
	WORLD* world = client->playerData->currentWorld;
	char* data = client->wrbuf;
	*data = 0x04;
	WriteShortVec(data, (SVECTOR*)world->dimensions);
	Client_Send(client, 7);
}

void Packet_WriteHandshake(CLIENT* client) {
	char* data = client->wrbuf;
	*data = 0x00;
	*++data = 0x07;
	WriteString(++data, "Server Name"); data += 63;
	WriteString(++data, "Server MOTD"); data += 63;
	*++data = 0x00;
	Client_Send(client, 131);
}

void Packet_WriteSpawn(CLIENT* client, CLIENT* other) {
	char* data = client->wrbuf;
	*data = 0x07;
	*++data = client == other ? 0xFF : other->id;
	WriteString(++data, other->playerData->name); data += 63;
	WriteClPos(++data, other, true);
	Client_Send(client, 74);
}

void Packet_WriteDespawn(CLIENT* client, CLIENT* other) {
	char* data = client->wrbuf;
	*data = 0x0C;
	*++data = client == other ? 0xFF : other->id;
	Client_Send(client, 2);
}

void Packet_WritePosAndOrient(CLIENT* client, CLIENT* other) {
	char* data = client->wrbuf;
	*data = 0x08;
	*++data = client == other ? 0xFF : other->id;
	WriteClPos(++data, other, false);
	Client_Send(client, 10);
}

void Packet_WriteChat(CLIENT* client, int type, char* mesg) {
	char* data = client->wrbuf;
	*data = 0x0D;
	*++data = type;
	WriteString(++data, mesg);
	Client_Send(client, 66);
}

bool Handler_Handshake(CLIENT* client, char* data) {
	uchar protoVer = *data;
	if(protoVer != 0x07) {
		Client_Kick(client, "Invalid protocol version");
		return true;
	}

	client->playerData = calloc(1, sizeof(struct playerData));
	client->playerData->currentWorld = worlds[0];
	client->playerData->position = calloc(1, sizeof(struct vector));
	client->playerData->angle = calloc(1, sizeof(struct angle));

	Client_SetPos(client, &worlds[0]->spawnVec, &worlds[0]->spawnAng);
	ReadString(++data, &client->playerData->name); data += 63;
	ReadString(++data, &client->playerData->key); data += 63;
	bool cpeEnabled = *++data == 0x42;

	if(Client_CheckAuth(client))
		Packet_WriteHandshake(client);
	else {
		Client_Kick(client, "Auth failed");
		return true;
	}

	if(cpeEnabled) {
		client->cpeData = calloc(1, sizeof(struct cpeData));
		CPE_StartHandshake(client);
	} else {
		Event_OnHandshakeDone(client);
		if(!Client_SendMap(client))
			Client_Kick(client, "Map sending failed");
	}

	return true;
}

bool Handler_SetBlock(CLIENT* client, char* data) {
	if(!client->playerData)
		return false;

	WORLD* world = client->playerData->currentWorld;
	if(!world) return false;

	ushort x = ntohs(*(ushort*)data); data += 2;
	ushort y = ntohs(*(ushort*)data); data += 2;
	ushort z = ntohs(*(ushort*)data); data += 2;
	uchar mode = *(uchar*)data; ++data;
	int block = (int)*(uchar*)data; ++data;

	switch(mode) {
		case MODE_PLACE:
			if(!Block_IsValid(block)) {
				Client_Kick(client, "Invalid block ID");
				return false;
			}
			if(Event_OnBlockPalce(client, x, y, z, block))
				World_SetBlock(world, x, y, z, block);
			break;
		case MODE_DESTROY:
			if(Event_OnBlockPalce(client, x, y, z, 0))
				World_SetBlock(world, x, y, z, 0);
			break;
	}

	return true;
}

bool Handler_PosAndOrient(CLIENT* client, char* data) {
	if(!client->playerData)
		return false;
	if(client->cpeData) {
		if(client->cpeData->heldBlock != *data) {
			Event_OnHeldBlockChange(client, client->cpeData->heldBlock, *data);
			client->cpeData->heldBlock = *data;
		}
	}
		client->cpeData->heldBlock = *data;
	ReadClPos(client, ++data);
	client->playerData->positionUpdated = true;
	return true;
}

bool Handler_Message(CLIENT* client, char* data) {
	if(!client->playerData)
		return false;

	char* message;
	int len = ReadString(++data, &message);

	for(int i = 0; i < 63; i++) {
		int nc = message[i + 1];
		if(message[i] == '%' && ISHEX(message[i + 1]))
			message[i] = '&';
	}

	if(Event_OnMessage(client, message, len)) {
		char formatted[128] = {0};
		sprintf(formatted, CHATLINE, client->playerData->name, message);

		if(*message == '/') {
			if(!Command_Handle(message + 1, client))
				Packet_WriteChat(client, 0, "Unknown command");
		} else
			Packet_WriteChat(Broadcast, 0, formatted);
		Log_Chat(formatted);
	}
	free(message);
	return true;
}
