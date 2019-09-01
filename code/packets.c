#include "core.h"
#include "log.h"
#include "block.h"
#include "world.h"
#include "client.h"
#include "event.h"
#include "server.h"
#include "packets.h"
#include "command.h"

PACKET* packets[256] = {0};

int ReadString(const char* data, char** dst) {
	int end = 63;
	while(data[end] == ' ') --end;
	++end;

	char* str = Memory_Alloc(end + 1, 1);
	Memory_Copy(str, data, end);
	str[end] = 0;
	dst[0] = str;
	return end;
}

void WriteString(char* data, const char* string) {
	uchar size = min((uchar)String_Length(string), 64);
	Memory_Fill(data + size, 64, 0);
	Memory_Copy(data, string, size);
}

void ReadClPos(CLIENT* client, char* data) {
	VECTOR* vec = client->playerData->position;
	ANGLE* ang = client->playerData->angle;

	vec->x = (float)ntohs(*(short*)data) / 32;++data;
	vec->y = (float)ntohs(*(short*)++data) / 32;++data;
	vec->z = (float)ntohs(*(short*)++data) / 32;++data;
	ang->yaw = (((float)(uchar)*++data) / 256) * 360;
	ang->pitch = (((float)(uchar)*++data) / 256) * 360;
}

char* WriteClPos(char* data, CLIENT* client, bool stand) {
	VECTOR* vec = client->playerData->position;
	ANGLE* ang = client->playerData->angle;

	*(ushort*)data = htons((ushort)(vec->x * 32));++data;
	*(ushort*)++data = htons((ushort)(vec->y * 32 + (stand ? 51 : 0)));++data;
	*(ushort*)++data = htons((ushort)(vec->z * 32));++data;
	*(uchar*)++data = (uchar)((ang->yaw / 360) * 256);
	*(uchar*)++data = (uchar)((ang->pitch / 360) * 256);

	return data;
}

void Packet_Register(int id, const char* name, ushort size, packetHandler handler) {
	PACKET* tmp = (PACKET*)Memory_Alloc(1, sizeof(PACKET));

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

short Packet_GetSize(int id, CLIENT* client) {
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
	char* data = client->wrbuf;
	*data = 0x0E;
	WriteString(++data, reason);
	Client_Send(client, 65);
}

void Packet_WriteLvlInit(CLIENT* client) {
	char* data = client->wrbuf;
	*data = 0x02;
	if(client->cpeData && client->cpeData->fmSupport) {
		*(uint*)++data = htonl(client->playerData->currentWorld->size - 4);
		Client_Send(client, 5);
	} else
		Client_Send(client, 1);
}

void Packet_WriteLvlFin(CLIENT* client) {
	WORLD* world = client->playerData->currentWorld;
	char* data = client->wrbuf;
	*data = 0x04;
	*(ushort*)++data = htons(world->info->dim->width);++data;
	*(ushort*)++data = htons(world->info->dim->height);++data;
	*(ushort*)++data = htons(world->info->dim->length);++data;
	Client_Send(client, 7);
}

void Packet_WriteSetBlock(CLIENT* client, ushort x, ushort y, ushort z, BlockID block) {
	char* data = client->wrbuf;
	*data = 0x06;
	*(ushort*)++data = htons(x);++data;
	*(ushort*)++data = htons(y);++data;
	*(ushort*)++data = htons(z);++data;
	*++data = block;
	Client_Send(client, 8);
}

void Packet_WriteHandshake(CLIENT* client, const char* name, const char* motd) {
	char* data = client->wrbuf;
	*data = 0x00;
	*++data = 0x07;
	WriteString(++data, name); data += 63;
	WriteString(++data, motd); data += 63;
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

void Packet_WriteChat(CLIENT* client, MessageType type, const char* mesg) {
	char* data = client->wrbuf;
	*data = 0x0D;
	*++data = type;
	WriteString(++data, mesg);
	Client_Send(client, 66);
}

bool Handler_Handshake(CLIENT* client, char* data) {
	uchar protoVer =* data;
	if(protoVer != 0x07) {
		Client_Kick(client, "Invalid protocol version");
		return true;
	}

	client->playerData = (PLAYERDATA*)Memory_Alloc(1, sizeof(PLAYERDATA));
	client->playerData->position = (VECTOR*)Memory_Alloc(1, sizeof(VECTOR));
	client->playerData->angle = (ANGLE*)Memory_Alloc(1, sizeof(ANGLE));

	Client_SetPos(client, worlds[0]->info->spawnVec, worlds[0]->info->spawnAng);
	ReadString(++data, (void*)&client->playerData->name); data += 63;
	ReadString(++data, (void*)&client->playerData->key); data += 63;
	Thread_SetName(client->playerData->name);

	for(int i = 0; i < 128; i++) {
		CLIENT* other = clients[i];
		if(!other || other == client) continue;
		if(String_CaselessCompare(client->playerData->name, other->playerData->name)) {
			Client_Kick(client, "This name already in use");
			return true;
		}
	}

	bool cpeEnabled =* ++data == 0x42;

	if(Client_CheckAuth(client)) {
		const char* name = Config_GetStr(mainCfg, "serverName");
		const char* motd = Config_GetStr(mainCfg, "serverMotd");

		if(!name)
			name = "Server name";
		if(!motd)
			motd = "Server motd";

		Packet_WriteHandshake(client, name, motd);
	} else {
		Client_Kick(client, "Auth failed");
		return true;
	}

	if(cpeEnabled) {
		client->cpeData = (CPEDATA*)Memory_Alloc(1, sizeof(CPEDATA));
		CPE_StartHandshake(client);
	} else {
		Event_OnHandshakeDone(client);
		Client_HandshakeStage2(client);
	}

	return true;
}

bool Handler_SetBlock(CLIENT* client, char* data) {
	if(!client->playerData || client->playerData->state != STATE_INGAME)
		return false;

	WORLD* world = client->playerData->currentWorld;
	if(!world) return false;

	ushort x = ntohs(*(ushort*)data); data += 2;
	ushort y = ntohs(*(ushort*)data); data += 2;
	ushort z = ntohs(*(ushort*)data); data += 2;
	uchar mode = *(uchar*)data; ++data;
	BlockID block = *(BlockID*)data;
	BlockID pblock = block;

	switch(mode) {
		case MODE_PLACE:
			if(!Block_IsValid(block)) {
				Client_Kick(client, "Invalid block ID");
				return false;
			}
			if(Event_OnBlockPlace(client, &x, &y, &z, &block)) {
				World_SetBlock(world, x, y, z, block);
				Client_UpdateBlock(pblock != block ? NULL : client, world, x, y, z);
			} else
				Packet_WriteSetBlock(client, x, y, z, World_GetBlock(world, x, y, z));
			break;
		case MODE_DESTROY:
			block = 0;
			if(Event_OnBlockPlace(client, &x, &y, &z, &block)) {
				World_SetBlock(world, x, y, z, block);
				Client_UpdateBlock(pblock != block ? NULL : client, world, x, y, z);
			} else
				Packet_WriteSetBlock(client, x, y, z, World_GetBlock(world, x, y, z));
			break;
	}

	return true;
}

bool Handler_PosAndOrient(CLIENT* client, char* data) {
	if(!client->playerData || client->playerData->state != STATE_INGAME)
		return false;

	if(client->cpeData) {
		if(client->cpeData->heldBlock !=* data) {
			BlockID id = *data;
			BlockID lastblock = client->cpeData->heldBlock;
			Event_OnHeldBlockChange(client, &lastblock, &id);
			client->cpeData->heldBlock = *data;
		}
	}
		client->cpeData->heldBlock = *data;
	ReadClPos(client, ++data);
	client->playerData->positionUpdated = true;
	return true;
}

bool Handler_Message(CLIENT* client, char* data) {
	if(!client->playerData || client->playerData->state != STATE_INGAME)
		return false;

	char* message;
	MessageType type = 0;
	int len = ReadString(++data, &message);

	for(int i = 0; i < len; i++) {
		if(message[i] == '%' && ISHEX(message[i + 1]))
			message[i] = '&';
	}

	if(Event_OnMessage(client, message, &type)) {
		char formatted[128] = {0};
		sprintf(formatted, CHATLINE, client->playerData->name, message);

		if(*message == '/') {
			if(!Command_Handle(message + 1, client))
				Packet_WriteChat(client, type, "Unknown command");
		} else
			Packet_WriteChat(Broadcast, type, formatted);
		Log_Chat(formatted);
	}

	Memory_Free(message);
	return true;
}
