#include "core.h"
#include "str.h"
#include "block.h"
#include "client.h"
#include "event.h"
#include "server.h"
#include "packets.h"
#include "platform.h"
#include "command.h"
#include "lang.h"

PACKET PackList[MAX_PACKETS] = {0};

uint8_t ReadNetString(const char** data, const char** dst) {
	const char* instr = *data;
	uint8_t end;

	for(end = 64; end > 0; end--) {
		if(end == 0) break;
		if(instr[end - 1] != ' ') break;
	}

	if(end > 0) {
		char* str = Memory_Alloc(end + 1, 1);
		Memory_Copy(str, instr, end);
		str[end] = '\0';
		dst[0] = str;
	};

	*data += 64;
	return end;
}

uint8_t ReadNetStringNoAlloc(const char** data, char* dst) {
	const char* instr = *data;
	uint8_t end;

	for(end = 64; end > 0; end--) {
		if(end == 0) break;
		if(instr[end - 1] != ' ') break;
	}

	if(end > 0) {
		Memory_Copy(dst, instr, end);
		dst[end] = '\0';
	}

	*data += 64;
	return end;
}

void WriteNetString(char* data, const char* string) {
	size_t size = min(String_Length(string), 64);
	if(size < 64) Memory_Fill(data + size, 64 - size, 32);
	Memory_Copy(data, string, size);
}

static bool ReadClPos(CLIENT client, const char* data) {
	PLAYERDATA cpd = client->playerData;
	VECTOR* vec = cpd->position;
	ANGLE* ang = cpd->angle;
	VECTOR newVec = {0};
	ANGLE newAng = {0};
	bool changed = false;

	if(Client_GetExtVer(client, EXT_ENTPOS)) {
		newVec.x = (float)ntohl(*(int*)data) / 32; data += 4;
		newVec.y = (float)ntohl(*(int*)data) / 32; data += 4;
		newVec.z = (float)ntohl(*(int*)data) / 32; data += 4;
		newAng.yaw = (((float)(uint8_t)*data++) / 256) * 360;
		newAng.pitch = (((float)(uint8_t)*data) / 256) * 360;
	} else {
		newVec.x = (float)ntohs(*(short*)data) / 32; data += 2;
		newVec.y = (float)ntohs(*(short*)data) / 32; data += 2;
		newVec.z = (float)ntohs(*(short*)data) / 32; data += 2;
		newAng.yaw = (((float)(uint8_t)*data++) / 256) * 360;
		newAng.pitch = (((float)(uint8_t)*data) / 256) * 360;
	}

	if(newVec.x != vec->x || newVec.y != vec->y || newVec.z != vec->z) {
		vec->x = newVec.x;
		vec->y = newVec.y;
		vec->z = newVec.z;
		Event_Call(EVT_ONPLAYERMOVE, client);
		changed = true;
	}

	if(newAng.yaw != ang->yaw || newAng.pitch != ang->pitch) {
		ang->yaw = newAng.yaw;
		ang->pitch = newAng.pitch;
		Event_Call(EVT_ONPLAYERROTATE, client);
		changed = true;
	}

	return changed;
}

static uint32_t WriteClPos(char* data, CLIENT client, bool stand, bool extended) {
	VECTOR* vec = client->playerData->position;
	ANGLE* ang = client->playerData->angle;

	uint32_t x = (uint32_t)(vec->x * 32), y = (uint32_t)(vec->y * 32 + (stand ? 51 : 0)), z = (uint32_t)(vec->z * 32);
	uint8_t yaw = (uint8_t)((ang->yaw / 360) * 256), pitch = (uint8_t)((ang->pitch / 360) * 256);

	if(extended) {
		*(uint32_t*)data = htonl((uint32_t)x); data += 4;
		*(uint32_t*)data = htonl((uint32_t)y); data += 4;
		*(uint32_t*)data = htonl((uint32_t)z); data += 4;
		*(uint8_t*)data++ = (uint8_t)yaw;
		*(uint8_t*)data = (uint8_t)pitch;
	} else {
		*(uint16_t*)data = htons((uint16_t)x); data += 2;
		*(uint16_t*)data = htons((uint16_t)y); data += 2;
		*(uint16_t*)data = htons((uint16_t)z); data += 2;
		*(uint8_t*)data++ = (uint8_t)yaw;
		*(uint8_t*)data = (uint8_t)pitch;
	}

	return extended ? 12 : 6;
}

void Packet_Register(int32_t id, const char* name, uint16_t size, packetHandler handler) {
	PACKET tmp = Memory_Alloc(1, sizeof(struct packet));

	tmp->name = name;
	tmp->size = size;
	tmp->handler = handler;
	PackList[id] = tmp;
}

void Packet_RegisterCPE(int32_t id, uint32_t extCRC32, int32_t version, uint16_t size, packetHandler handler) {
	PACKET tmp = PackList[id];

	tmp->extCRC32 = extCRC32;
	tmp->extVersion = version;
	tmp->cpeHandler = handler;
	tmp->extSize = size;
	tmp->haveCPEImp = true;
}

void Packet_RegisterDefault(void) {
	Packet_Register(0x00, "Handshake", 130, Handler_Handshake);
	Packet_Register(0x05, "SetBlock", 8, Handler_SetBlock);
	Packet_Register(0x08, "PosAndOrient", 9, Handler_PosAndOrient);
	Packet_Register(0x0D, "Message", 65, Handler_Message);
}

PACKET Packet_Get(int32_t id) {
	return id < MAX_PACKETS ? PackList[id] : NULL;
}

/*
	VANILLA
*/

void Packet_WriteHandshake(CLIENT client, const char* name, const char* motd) {
	PacketWriter_Start(client);

	*data++ = 0x00;
	*data++ = 0x07;
	WriteNetString(data++, name); data += 64;
	WriteNetString(data++, motd); data += 64;
	*data = (char)client->playerData->isOP;

	PacketWriter_End(client, 131);
}

void Packet_WriteLvlInit(CLIENT client) {
	PacketWriter_Start(client);

	*data++ = 0x02;
	if(client->cpeData && Client_GetExtVer(client, EXT_FASTMAP)) {
		*(uint32_t*)data = htonl(client->playerData->world->size - 4);
		PacketWriter_End(client, 5);
	} else {
		PacketWriter_End(client, 1);
	}
}

void Packet_WriteLvlFin(CLIENT client) {
	PacketWriter_Start(client);

	WORLDDIMS dims = client->playerData->world->info->dim;
	*data++ = 0x04;
	*(uint16_t*)data = htons(dims->width); data += 2;
	*(uint16_t*)data = htons(dims->height); data += 2;
	*(uint16_t*)data = htons(dims->length); data += 2;

	PacketWriter_End(client, 7);
}

void Packet_WriteSetBlock(CLIENT client, uint16_t x, uint16_t y, uint16_t z, BlockID block) {
	PacketWriter_Start(client);

	*data++ = 0x06;
	*(uint16_t*)data = htons(x); data += 2;
	*(uint16_t*)data = htons(y); data += 2;
	*(uint16_t*)data = htons(z); data += 2;
	*data = block;

	PacketWriter_End(client, 8);
}

void Packet_WriteSpawn(CLIENT client, CLIENT other) {
	PacketWriter_Start(client);

	*data++ = 0x07;
	*data++ = client == other ? 0xFF : other->id;
	WriteNetString(data, other->playerData->name); data += 64;
	bool extended = Client_GetExtVer(client, EXT_ENTPOS);
	uint32_t len = WriteClPos(data, other, client == other, extended);

	PacketWriter_End(client, 68 + len);
}

void Packet_WritePosAndOrient(CLIENT client, CLIENT other) {
	PacketWriter_Start(client);

	*data++ = 0x08;
	*data++ = client == other ? 0xFF : other->id;
	bool extended = Client_GetExtVer(client, EXT_ENTPOS);
	uint32_t len = WriteClPos(data, other, false, extended);

	PacketWriter_End(client, 4 + len);
}

void Packet_WriteDespawn(CLIENT client, CLIENT other) {
	PacketWriter_Start(client);

	*data++ = 0x0C;
	*data = client == other ? 0xFF : other->id;

	PacketWriter_End(client, 2);
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

	if(!Client_GetExtVer(client, EXT_CP437)) {
		for(int32_t i = 0; i < 64; i++) {
			if(mesg_out[i] == '\0') break;
			if(mesg_out[i] < ' ' || mesg_out[i] > '~')
				mesg_out[i] = '?';
		}
	}

	*data++ = 0x0D;
	*data++ = type;
	WriteNetString(data, mesg_out);

	PacketWriter_End(client, 66);
}

void Packet_WriteKick(CLIENT client, const char* reason) {
	PacketWriter_Start(client);

	*data++ = 0x0E;
	WriteNetString(data, reason);

	PacketWriter_End(client, 65);
}

/*
	Classic handlers
*/

bool Handler_Handshake(CLIENT client, const char* data) {
	uint8_t protoVer = *data++;
	if(protoVer != 0x07) {
		Client_Kick(client, Lang_Get(LANG_KICKPROTOVER));
		return true;
	}

	client->playerData = Memory_Alloc(1, sizeof(struct playerData));
	client->playerData->position = Memory_Alloc(1, sizeof(struct vector));
	client->playerData->angle = Memory_Alloc(1, sizeof(struct angle));

	if(client->addr == INADDR_LOOPBACK && Config_GetBool(Server_Config, CFG_LOCALOP_KEY))
		client->playerData->isOP = true;

	if(!ReadNetString(&data, &client->playerData->name)) return false;
	if(!ReadNetString(&data, &client->playerData->key)) return false;

	for(int32_t i = 0; i < 128; i++) {
		CLIENT other = Clients_List[i];
		if(!other || other == client || !other->playerData) continue;
		if(String_CaselessCompare(client->playerData->name, other->playerData->name)) {
			Client_Kick(client, Lang_Get(LANG_KICKNAMEUSED));
			return true;
		}
	}

	if(Client_CheckAuth(client)) {
		const char* name = Config_GetStr(Server_Config, CFG_SERVERNAME_KEY);
		const char* motd = Config_GetStr(Server_Config, CFG_SERVERMOTD_KEY);

		Packet_WriteHandshake(client, name, motd);
	} else {
		Client_Kick(client, Lang_Get(LANG_KICKAUTHFAIL));
		return true;
	}

	if(*data == 0x42) {
		client->cpeData = Memory_Alloc(1, sizeof(struct cpeData));
		client->cpeData->model = 256; // Humanoid model id
		CPE_StartHandshake(client);
	} else {
		Event_Call(EVT_ONHANDSHAKEDONE, (void*)client);
		Client_HandshakeStage2(client);
	}

	return true;
}

static void UpdateBlock(WORLD world, uint16_t x, uint16_t y, uint16_t z, BlockID block) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT client = Clients_List[i];
		if(client && Client_IsInGame(client) && Client_IsInWorld(client, world))
			Packet_WriteSetBlock(client, x, y, z, block);
	}
}

bool Handler_SetBlock(CLIENT client, const char* data) {
	ValidateClientState(client, STATE_INGAME, false);

	WORLD world = client->playerData->world;
	if(!world) return false;

	uint16_t x = ntohs(*(uint16_t*)data); data += 2;
	uint16_t y = ntohs(*(uint16_t*)data); data += 2;
	uint16_t z = ntohs(*(uint16_t*)data); data += 2;
	uint8_t mode = *(uint8_t*)data++;
	BlockID block = *(BlockID*)data;

	switch(mode) {
		case 0x01:
			if(!Block_IsValid(block)) {
				Client_Kick(client, Lang_Get(LANG_KICKBLOCKID));
				return false;
			}
			if(Event_OnBlockPlace(client, mode, x, y, z, &block)) {
				World_SetBlock(world, x, y, z, block);
				UpdateBlock(world, x, y, z, block);
			} else
				Packet_WriteSetBlock(client, x, y, z, World_GetBlock(world, x, y, z));
			break;
		case 0x00:
			block = BLOCK_AIR;
			if(Event_OnBlockPlace(client, mode, x, y, z, &block)) {
				World_SetBlock(world, x, y, z, block);
				UpdateBlock(world, x, y, z, block);
			} else
				Packet_WriteSetBlock(client, x, y, z, World_GetBlock(world, x, y, z));
			break;
	}

	return true;
}

bool Handler_PosAndOrient(CLIENT client, const char* data) {
	ValidateClientState(client, STATE_INGAME, false);
	CPEDATA cpd = client->cpeData;
	BlockID cb = *data++;

	if(cpd && cpd->heldBlock != cb) {
		BlockID new = cb;
		BlockID curr = cpd->heldBlock;
		Event_OnHeldBlockChange(client, curr, new);
		cpd->heldBlock = new;
	}

	if(ReadClPos(client, data))
		Client_UpdatePositions(client);
	return true;
}

bool Handler_Message(CLIENT client, const char* data) {
	ValidateClientState(client, STATE_INGAME, true);

	MessageType type = 0;
	char message[65];
	char* messptr = message;
	uint8_t partial = *data++;
	uint8_t len = ReadNetStringNoAlloc(&data, message);
	if(len == 0) return false;

	for(uint8_t i = 0; i < len; i++) {
		if(message[i] == '%' && ISHEX(message[i + 1]))
			message[i] = '&';
	}

	CPEDATA cpd = client->cpeData;
	if(cpd && Client_GetExtVer(client, EXT_LONGMSG)) {
		if(String_Append(cpd->message, 193, message) && partial == 1) return true;
		messptr = cpd->message;
	}

	if(Event_OnMessage(client, messptr, &type)) {
		char formatted[320] = {0};
		String_FormatBuf(formatted, 320, CHATLINE, client->playerData->name, messptr);

		if(*messptr == '/') {
			if(!Command_Handle(messptr, client))
				Packet_WriteChat(client, type, Lang_Get(LANG_CMDUNK));
		} else
			Client_Chat(Client_Broadcast, type, formatted);

		Log_Chat(formatted);
	}

	if(messptr != message) *messptr = '\0';

	return true;
}
