#include "core.h"
#include "world.h"
#include "client.h"
#include "event.h"
#include "server.h"
#include "packets.h"
#include "command.h"

PACKET Packets_List[MAX_PACKETS] = {0};

uint8_t ReadNetString(const char* data, const char** dst) {
	uint8_t end;
	for(end = 63; end > 0; --end) {
		if(end == 0) break;
		if(data[end - 1] != ' ') break;
	}

	char* str = Memory_Alloc(end + 1, 1);
	Memory_Copy(str, data, end);
	str[end] = '\0';
	dst[0] = str;

	return end;
}

void WriteNetString(char* data, const char* string) {
	size_t size = min(String_Length(string), 64);
	Memory_Fill(data + size, 64, 0);
	Memory_Copy(data, string, size);
}

void ReadClPos(CLIENT client, char* data, bool extended) {
	VECTOR vec = client->playerData->position;
	ANGLE ang = client->playerData->angle;

	if(extended) {
		vec->x = (float)ntohl(*(int*)data) / 32; data += 3;
		vec->y = (float)ntohl(*(int*)++data) / 32; data += 3;
		vec->z = (float)ntohl(*(int*)++data) / 32; data += 3;
		ang->yaw = (((float)(uint8_t)*++data) / 256) * 360;
		ang->pitch = (((float)(uint8_t)*++data) / 256) * 360;
	} else {
		vec->x = (float)ntohs(*(short*)data) / 32; ++data;
		vec->y = (float)ntohs(*(short*)++data) / 32; ++data;
		vec->z = (float)ntohs(*(short*)++data) / 32; ++data;
		ang->yaw = (((float)(uint8_t)*++data) / 256) * 360;
		ang->pitch = (((float)(uint8_t)*++data) / 256) * 360;
	}
}

uint32_t WriteClPos(char* data, CLIENT client, bool stand, bool extended) {
	VECTOR vec = client->playerData->position;
	ANGLE ang = client->playerData->angle;

	uint32_t x = (uint32_t)(vec->x * 32), y = (uint32_t)(vec->y * 32 + (stand ? 51 : 0)), z = (uint32_t)(vec->z * 32);
	uint8_t yaw = (uint8_t)((ang->yaw / 360) * 256), pitch = (uint8_t)((ang->pitch / 360) * 256);

	if(extended) {
		*(uint32_t*)data = htonl((uint32_t)x); data += 3;
		*(uint32_t*)++data = htonl((uint32_t)y); data += 3;
		*(uint32_t*)++data = htonl((uint32_t)z); data += 3;
		*(uint8_t*)++data = (uint8_t)yaw;
		*(uint8_t*)++data = (uint8_t)pitch;
	} else {
		*(uint16_t*)data = htons((uint16_t)x); ++data;
		*(uint16_t*)++data = htons((uint16_t)y); ++data;
		*(uint16_t*)++data = htons((uint16_t)z); ++data;
		*(uint8_t*)++data = (uint8_t)yaw;
		*(uint8_t*)++data = (uint8_t)pitch;
	}

	return extended ? 12 : 6;
}

void Packet_Register(int id, const char* name, uint16_t size, packetHandler handler) {
	PACKET tmp = Memory_Alloc(1, sizeof(struct packet));

	tmp->name = name;
	tmp->size = size;
	tmp->handler = handler;
	Packets_List[id] = tmp;
}

void Packet_RegisterCPE(int id, uint32_t extCRC32, int version, uint16_t size, packetHandler handler) {
	PACKET tmp = Packets_List[id];

	tmp->extCRC32 = extCRC32;
	tmp->extVersion = version;
	tmp->cpeHandler = handler;
	tmp->extSize = size;
	tmp->haveCPEImp = true;
}

void Packet_RegisterDefault(void) {
	Packet_Register(0x00, "Handshake", 131, Handler_Handshake);
	Packet_Register(0x05, "SetBlock", 9, Handler_SetBlock);
	Packet_Register(0x08, "PosAndOrient", 10, Handler_PosAndOrient);
	Packet_Register(0x0D, "Message", 66, Handler_Message);
}

PACKET Packet_Get(int id) {
	return id < MAX_PACKETS ? Packets_List[id] : NULL;
}

/*
	VANILLA
*/

void Packet_WriteKick(CLIENT client, const char* reason) {
	PacketWriter_Start(client);

	*data = 0x0E;
	WriteNetString(++data, reason);

	PacketWriter_End(client, 65);
}

void Packet_WriteLvlInit(CLIENT client) {
	PacketWriter_Start(client);

	*data = 0x02;
	if(client->cpeData && Client_IsSupportExt(client, EXT_FASTMAP, 1)) {
		*(uint32_t*)++data = htonl(client->playerData->world->size - 4);
		PacketWriter_End(client, 5);
	} else {
		PacketWriter_End(client, 1);
	}
}

void Packet_WriteLvlFin(CLIENT client) {
	PacketWriter_Start(client);

	WORLDDIMS dims = client->playerData->world->info->dim;
	*data = 0x04;
	*(uint16_t*)++data = htons(dims->width); ++data;
	*(uint16_t*)++data = htons(dims->height); ++data;
	*(uint16_t*)++data = htons(dims->length); ++data;

	PacketWriter_End(client, 7);
}

void Packet_WriteSetBlock(CLIENT client, uint16_t x, uint16_t y, uint16_t z, BlockID block) {
	PacketWriter_Start(client);

	*data = 0x06;
	*(uint16_t*)++data = htons(x); ++data;
	*(uint16_t*)++data = htons(y); ++data;
	*(uint16_t*)++data = htons(z); ++data;
	*++data = block;

	PacketWriter_End(client, 8);
}

void Packet_WriteHandshake(CLIENT client, const char* name, const char* motd) {
	PacketWriter_Start(client);

	*data = 0x00;
	*++data = 0x07;
	WriteNetString(++data, name); data += 63;
	WriteNetString(++data, motd); data += 63;
	*++data = (char)client->playerData->isOP;

	PacketWriter_End(client, 131);
}

void Packet_WriteSpawn(CLIENT client, CLIENT other) {
	PacketWriter_Start(client);

	*data = 0x07;
	*++data = client == other ? 0xFF : other->id;
	WriteNetString(++data, other->playerData->name); data += 63;
	bool extended = Client_IsSupportExt(client, EXT_ENTPOS, 1);
	uint32_t len = WriteClPos(++data, other, true, extended);

	PacketWriter_End(client, 68 + len);
}

void Packet_WriteDespawn(CLIENT client, CLIENT other) {
	PacketWriter_Start(client);

	*data = 0x0C;
	*++data = client == other ? 0xFF : other->id;

	PacketWriter_End(client, 2);
}

void Packet_WritePosAndOrient(CLIENT client, CLIENT other) {
	PacketWriter_Start(client);

	*data = 0x08;
	*++data = client == other ? 0xFF : other->id;
	bool extended = Client_IsSupportExt(client, EXT_ENTPOS, 1);
	uint32_t len = WriteClPos(++data, other, false, extended);

	PacketWriter_End(client, 4 + len);
}

void Packet_WriteChat(CLIENT client, MessageType type, const char* mesg) {
	PacketWriter_Start(client);
	if(client == Client_Broadcast) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			CLIENT tg = Clients_List[i];
			if(tg) Packet_WriteChat(tg, type, mesg);
		}
		PacketWriter_Stop(client);
		return;
	}

	char mesg_out[64] = {0};
	String_Copy(mesg_out, 64, mesg);

	if(!Client_IsSupportExt(client, EXT_CP437, 1)) {
		for(int i = 0; i < 64; i++) {
			if(mesg_out[i] == '\0') break;
			if(mesg_out[i] < ' ' || mesg_out[i] > '~')
				mesg_out[i] = '?';
		}
	}

	*data = 0x0D;
	*++data = type;
	WriteNetString(++data, mesg_out);

	PacketWriter_End(client, 66);
}

void Packet_WriteUpdateType(CLIENT client) {
	PacketWriter_Start(client);

	*data = 0x0F;
	*++data = client->playerData->isOP ? 0x64 : 0x00;

	PacketWriter_End(client, 2);
}

/*
	Classic handlers
*/

bool Handler_Handshake(CLIENT client, char* data) {
	uint8_t protoVer =* data;
	if(protoVer != 0x07) {
		Client_Kick(client, "Invalid protocol version");
		return true;
	}

	client->playerData = Memory_Alloc(1, sizeof(struct playerData));
	client->playerData->position = Memory_Alloc(1, sizeof(struct vector));
	client->playerData->angle = Memory_Alloc(1, sizeof(struct angle));

	if(client->addr == INADDR_LOOPBACK && Config_GetBool(Server_Config, "alwayslocalop"))
		client->playerData->isOP = true;

	ReadNetString(++data, &client->playerData->name); data += 63;
	ReadNetString(++data, &client->playerData->key); data += 63;

	for(int i = 0; i < 128; i++) {
		CLIENT other = Clients_List[i];
		if(!other || other == client) continue;
		if(String_CaselessCompare(client->playerData->name, other->playerData->name)) {
			Client_Kick(client, "This name already in use");
			return true;
		}
	}

	bool cpeEnabled = *++data == 0x42;

	if(Client_CheckAuth(client)) {
		const char* name = Config_GetStr(Server_Config, "name");
		const char* motd = Config_GetStr(Server_Config, "motd");

		if(!name)
			name = DEFAULT_NAME;
		if(!motd)
			motd = DEFAULT_MOTD;

		Packet_WriteHandshake(client, name, motd);
	} else {
		Client_Kick(client, "Auth failed");
		return true;
	}

	if(cpeEnabled) {
		client->cpeData = Memory_Alloc(1, sizeof(struct cpeData));
		String_CopyUnsafe(client->cpeData->model, "humanoid");
		CPE_StartHandshake(client);
	} else {
		Event_Call(EVT_ONHANDSHAKEDONE, (void*)client);
		Client_HandshakeStage2(client);
	}

	return true;
}

static void UpdateBlock(CLIENT client, WORLD world, uint16_t x, uint16_t y, uint16_t z) {
	BlockID block = World_GetBlock(world, x, y, z);

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT other = Clients_List[i];
		if(!other || other == client) continue;
		if(!Client_IsInGame(other) || !Client_IsInWorld(other, world)) continue;
		Packet_WriteSetBlock(other, x, y, z, block);
	}
}

bool Handler_SetBlock(CLIENT client, char* data) {
	ValidateClientState(client, STATE_INGAME, false);

	WORLD world = client->playerData->world;
	if(!world) return false;

	uint16_t x = ntohs(*(uint16_t*)data); data += 2;
	uint16_t y = ntohs(*(uint16_t*)data); data += 2;
	uint16_t z = ntohs(*(uint16_t*)data); data += 2;
	uint8_t mode = *(uint8_t*)data; ++data;
	BlockID block = *(BlockID*)data;
	BlockID pblock = block;

	switch(mode) {
		case 0x01:
			if(!Block_IsValid(block)) {
				Client_Kick(client, "Invalid block ID");
				return false;
			}
			if(Event_OnBlockPlace(client, &x, &y, &z, &block)) {
				World_SetBlock(world, x, y, z, block);
				UpdateBlock(pblock != block ? NULL : client, world, x, y, z);
			} else
				Packet_WriteSetBlock(client, x, y, z, World_GetBlock(world, x, y, z));
			break;
		case 0x00:
			block = 0;
			if(Event_OnBlockPlace(client, &x, &y, &z, &block)) {
				World_SetBlock(world, x, y, z, block);
				UpdateBlock(pblock != block ? NULL : client, world, x, y, z);
			} else
				Packet_WriteSetBlock(client, x, y, z, World_GetBlock(world, x, y, z));
			break;
	}

	return true;
}

bool Handler_PosAndOrient(CLIENT client, char* data) {
	ValidateClientState(client, STATE_INGAME, false);

	if(client->cpeData && client->cpeData->heldBlock != *data) {
		BlockID new = *data;
		BlockID curr = client->cpeData->heldBlock;
		Event_OnHeldBlockChange(client, &curr, &new);
		client->cpeData->heldBlock = *data;
	}

	ReadClPos(client, ++data, false);
	Client_UpdatePositions(client);
	return true;
}

bool Handler_Message(CLIENT client, char* data) {
	ValidateClientState(client, STATE_INGAME, true);

	char* message;
	MessageType type = 0;
	uint8_t len = ReadNetString(++data, (const char**)&message);

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
			Packet_WriteChat(Client_Broadcast, type, formatted);
		Log_Chat(formatted);
	}

	Memory_Free(message);
	return true;
}
