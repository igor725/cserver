#include "core.h"
#include "str.h"
#include "log.h"
#include "block.h"
#include "client.h"
#include "event.h"
#include "server.h"
#include "protocol.h"
#include "platform.h"
#include "command.h"
#include "lang.h"

Packet PackList[MAX_PACKETS] = {0};
cs_uint16 extensionsCount;
CPEExt firstExtension;

void Proto_WriteString(char** dataptr, const char* string) {
	char* data = *dataptr;
	cs_size size = 0;
	if(string) {
		size = min(String_Length(string), 64);
		Memory_Copy(data, string, size);
	}
	if(size < 64)
		Memory_Fill(data + size, 64 - size, 32);
	*dataptr += 64;
}

void Proto_WriteFlVec(char** dataptr, const Vec* vec) {
	char* data = *dataptr;
	*(cs_int32*)data = htonl((cs_int32)(vec->x * 32)); data += 4;
	*(cs_int32*)data = htonl((cs_int32)(vec->y * 32)); data += 4;
	*(cs_int32*)data = htonl((cs_int32)(vec->z * 32)); data += 4;
	*dataptr = data;
}

void Proto_WriteFlSVec(char** dataptr, const Vec* vec) {
	char* data = *dataptr;
	*(cs_int16*)data = htons((cs_int16)(vec->x * 32)); data += 2;
	*(cs_int16*)data = htons((cs_int16)(vec->y * 32)); data += 2;
	*(cs_int16*)data = htons((cs_int16)(vec->z * 32)); data += 2;
	*dataptr = data;
}

void Proto_WriteSVec(char** dataptr, const SVec* vec) {
	char* data = *dataptr;
	*(cs_int16*)data = htons(vec->x); data += 2;
	*(cs_int16*)data = htons(vec->y); data += 2;
	*(cs_int16*)data = htons(vec->z); data += 2;
	*dataptr = data;
}

void Proto_WriteAng(char** dataptr, const Ang* ang) {
	char* data = *dataptr;
	*(cs_uint8*)data++ = (cs_uint8)((ang->yaw / 360) * 256);
	*(cs_uint8*)data++ = (cs_uint8)((ang->pitch / 360) * 256);
	*dataptr = data;
}

void Proto_WriteColor3(char** dataptr, const Color3* color) {
	char* data = *dataptr;
	*(cs_int16*)data = htons(color->r); data += 2;
	*(cs_int16*)data = htons(color->g); data += 2;
	*(cs_int16*)data = htons(color->b); data += 2;
	*dataptr = data;
}

void Proto_WriteColor4(char** dataptr, const Color4* color) {
	char* data = *dataptr;
	*(cs_int16*)data = htons(color->r); data += 2;
	*(cs_int16*)data = htons(color->g); data += 2;
	*(cs_int16*)data = htons(color->b); data += 2;
	*(cs_int16*)data = htons(color->a); data += 2;
	*dataptr = data;
}

cs_uint32 Proto_WriteClientPos(char* data, Client client, cs_bool extended) {
	PlayerData pd = client->playerData;

	if(extended)
		Proto_WriteFlVec(&data, &pd->position);
	else
		Proto_WriteFlSVec(&data, &pd->position);

	Proto_WriteAng(&data, &pd->angle);

	return extended ? 12 : 6;
}

cs_uint8 Proto_ReadString(const char** dataptr, const char** dst) {
	const char* data = *dataptr;
	*dataptr += 64;
	cs_uint8 end;

	for(end = 64; end > 0; end--) {
		if(data[end - 1] != ' ')
			break;
	}

	if(end > 0) {
		char* str = Memory_Alloc(end + 1, 1);
		Memory_Copy(str, data, end);
		str[end] = '\0';
		dst[0] = str;
	};


	return end;
}

cs_uint8 Proto_ReadStringNoAlloc(const char** dataptr, char* dst) {
	const char* data = *dataptr;
	*dataptr += 64;
	cs_uint8 end;

	for(end = 64; end > 0; end--) {
		if(data[end - 1] != ' ')
			break;
	}

	if(end > 0) {
		Memory_Copy(dst, data, end);
		dst[end] = '\0';
	}

	return end;
}

void Proto_ReadSVec(const char** dataptr, SVec* vec) {
	const char* data = *dataptr;
	vec->x = ntohs(*(cs_int16*)data); data += 2;
	vec->y = ntohs(*(cs_int16*)data); data += 2;
	vec->z = ntohs(*(cs_int16*)data); data += 2;
	*dataptr = data;
}

void Proto_ReadAng(const char** dataptr, Ang* ang) {
	const char* data = *dataptr;
	ang->yaw = (((float)(cs_uint8)*data++) / 256) * 360;
	ang->pitch = (((float)(cs_uint8)*data++) / 256) * 360;
	*dataptr = data;
}

void Proto_ReadFlSVec(const char** dataptr, Vec* vec) {
	const char* data = *dataptr;
	vec->x = (float)ntohs(*(cs_int16*)data) / 32; data += 2;
	vec->y = (float)ntohs(*(cs_int16*)data) / 32; data += 2;
	vec->z = (float)ntohs(*(cs_int16*)data) / 32; data += 2;
	*dataptr = data;
}

void Proto_ReadFlVec(const char** dataptr, Vec* vec) {
	const char* data = *dataptr;
	vec->x = (float)ntohl(*(cs_int32*)data) / 32; data += 4;
	vec->y = (float)ntohl(*(cs_int32*)data) / 32; data += 4;
	vec->z = (float)ntohl(*(cs_int32*)data) / 32; data += 4;
	*dataptr = data;
}

cs_bool Proto_ReadClientPos(Client client, const char* data) {
	PlayerData cpd = client->playerData;
	Vec* vec = &cpd->position;
	Ang* ang = &cpd->angle;
	Vec newVec = {0};
	Ang newAng = {0};
	cs_bool changed = false;

	if(Client_GetExtVer(client, EXT_ENTPOS))
		Proto_ReadFlVec(&data, &newVec);
	else
		Proto_ReadFlSVec(&data, &newVec);

	Proto_ReadAng(&data, &newAng);

	if(newVec.x != vec->x || newVec.y != vec->y || newVec.z != vec->z) {
		cpd->position = newVec;
		Event_Call(EVT_ONPLAYERMOVE, client);
		changed = true;
	}

	if(newAng.yaw != ang->yaw || newAng.pitch != ang->pitch) {
		cpd->angle = newAng;
		Event_Call(EVT_ONPLAYERROTATE, client);
		changed = true;
	}

	return changed;
}

void Packet_Register(cs_int32 id, cs_uint16 size, packetHandler handler) {
	Packet tmp = Memory_Alloc(1, sizeof(struct packet));
	tmp->size = size;
	tmp->handler = handler;
	PackList[id] = tmp;
}

void Packet_RegisterCPE(cs_int32 id, cs_uint32 extCRC32, cs_int32 version, cs_uint16 size, packetHandler handler) {
	Packet tmp = PackList[id];
	tmp->extCRC32 = extCRC32;
	tmp->extVersion = version;
	tmp->cpeHandler = handler;
	tmp->extSize = size;
	tmp->haveCPEImp = true;
}

void Packet_RegisterExtension(const char* name, cs_int32 version) {
	CPEExt tmp = Memory_Alloc(1, sizeof(struct cpeExt));

	tmp->name = name;
	tmp->version = version;
	tmp->next = firstExtension;
	firstExtension = tmp;
	++extensionsCount;
}

struct extReg {
	const char* name;
	cs_int32 version;
};

static const struct extReg serverExtensions[] = {
	{"ClickDistance", 1},
	// CustomBlocks
	{"HeldBlock", 1},
	{"EmoteFix", 1},
	{"TextHotKey", 1},
	{"ExtPlayerList", 2},
	{"EnvColors", 1},
	{"SelectionCuboid", 1},
	{"BlockPermissions", 1},
	{"ChangeModel", 1},
	// EnvMapAppearance
	{"EnvWeatherType", 1},
	{"HackControl", 1},
	{"MessageTypes", 1},
	{"PlayerClick", 1},
	{"LongerMessages", 1},
	{"FullCP437", 1},
	// BlockDefinitions
	// BlockDefinitionsExt
	// BulkBlockUpdate
	// TextColors
	{"EnvMapAspect", 1},
	// EntityProperty
	{"ExtEntityPositions", 1},
	{"TwoWayPing", 1},
	{"InventoryOrder", 1},
	// InstantMOTD
	{"FastMap", 1},
	{"SetHotbar", 1},
	{NULL, 0}
};

void Packet_RegisterDefault(void) {
	Packet_Register(0x00, 130, Handler_Handshake);
	Packet_Register(0x05,   8, Handler_SetBlock);
	Packet_Register(0x08,   9, Handler_PosAndOrient);
	Packet_Register(0x0D,  65, Handler_Message);

	const struct extReg* ext;
	for(ext = serverExtensions; ext->name; ext++) {
		Packet_RegisterExtension(ext->name, ext->version);
	}

	Packet_Register(0x10, 66, CPEHandler_ExtInfo);
	Packet_Register(0x11, 68, CPEHandler_ExtEntry);
	Packet_Register(0x2B,  3, CPEHandler_TwoWayPing);
	Packet_Register(0x22, 14, CPEHandler_PlayerClick);
	Packet_RegisterCPE(0x08, EXT_ENTPOS, 1, 15, NULL);
}

Packet Packet_Get(cs_int32 id) {
	return id < MAX_PACKETS ? PackList[id] : NULL;
}

/*
** Врайтеры и хендлеры
** ванильного протокола
*/

void Packet_WriteHandshake(Client client, const char* name, const char* motd) {
	PacketWriter_Start(client);

	*data++ = 0x00;
	*data++ = 0x07;
	Proto_WriteString(&data, name);
	Proto_WriteString(&data, motd);
	*data = (char)client->playerData->isOP;

	PacketWriter_End(client, 131);
}

void Packet_WriteLvlInit(Client client) {
	PacketWriter_Start(client);

	*data++ = 0x02;
	if(Client_GetExtVer(client, EXT_FASTMAP)) {
		*(cs_uint32*)data = htonl(client->playerData->world->size - 4);
		PacketWriter_End(client, 5);
	} else {
		PacketWriter_End(client, 1);
	}
}

void Packet_WriteLvlFin(Client client, SVec* dims) {
	PacketWriter_Start(client);

	*data++ = 0x04;
	Proto_WriteSVec(&data, dims);

	PacketWriter_End(client, 7);
}

void Packet_WriteSetBlock(Client client, SVec* pos, BlockID block) {
	PacketWriter_Start(client);

	*data++ = 0x06;
	Proto_WriteSVec(&data, pos);
	*data = block;

	PacketWriter_End(client, 8);
}

void Packet_WriteSpawn(Client client, Client other) {
	PacketWriter_Start(client);

	*data++ = 0x07;
	*data++ = client == other ? 0xFF : other->id;
	Proto_WriteString(&data, other->playerData->name);
	cs_bool extended = Client_GetExtVer(client, EXT_ENTPOS) != 0;
	cs_uint32 len = Proto_WriteClientPos(data, other, extended);

	PacketWriter_End(client, 68 + len);
}

void Packet_WritePosAndOrient(Client client, Client other) {
	PacketWriter_Start(client);

	*data++ = 0x08;
	*data++ = client == other ? 0xFF : other->id;
	cs_bool extended = Client_GetExtVer(client, EXT_ENTPOS) != 0;
	cs_uint32 len = Proto_WriteClientPos(data, other, extended);

	PacketWriter_End(client, 4 + len);
}

void Packet_WriteDespawn(Client client, Client other) {
	PacketWriter_Start(client);

	*data++ = 0x0C;
	*data = client == other ? 0xFF : other->id;

	PacketWriter_End(client, 2);
}

void Packet_WriteChat(Client client, MessageType type, const char* mesg) {
	PacketWriter_Start(client);
	if(client == Client_Broadcast) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client tg = Clients_List[i];
			if(tg) Packet_WriteChat(tg, type, mesg);
		}
		PacketWriter_Stop(client);
		return;
	}

	char mesg_out[64] = {0};
	String_Copy(mesg_out, 64, mesg);

	if(!Client_GetExtVer(client, EXT_CP437)) {
		for(cs_int32 i = 0; i < 64; i++) {
			if(mesg_out[i] == '\0') break;
			if(mesg_out[i] < ' ' || mesg_out[i] > '~')
				mesg_out[i] = '?';
		}
	}

	*data++ = 0x0D;
	*data++ = type;
	Proto_WriteString(&data, mesg_out);

	PacketWriter_End(client, 66);
}

void Packet_WriteKick(Client client, const char* reason) {
	PacketWriter_Start(client);

	*data++ = 0x0E;
	Proto_WriteString(&data, reason);

	PacketWriter_End(client, 65);
}

cs_bool Handler_Handshake(Client client, const char* data) {
	if(*data++ != 0x07) {
		Client_Kick(client, Lang_Get(LANG_KICKPROTOVER));
		return true;
	}

	client->playerData = Memory_Alloc(1, sizeof(struct playerData));
	client->playerData->firstSpawn = true;
	if(client->addr == INADDR_LOOPBACK && Config_GetBool(Server_Config, CFG_LOCALOP_KEY))
		client->playerData->isOP = true;

	if(!Proto_ReadString(&data, &client->playerData->name)) return false;
	if(!Proto_ReadString(&data, &client->playerData->key)) return false;

	for(cs_int32 i = 0; i < MAX_CLIENTS; i++) {
		Client other = Clients_List[i];
		if(!other || !other->playerData || other == client) continue;
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

		CPEPacket_WriteInfo(client);
		CPEExt ptr = firstExtension;
		while(ptr) {
			CPEPacket_WriteExtEntry(client, ptr);
			ptr = ptr->next;
		}
	} else {
		Event_Call(EVT_ONHANDSHAKEDONE, (void*)client);
		Client_HandshakeStage2(client);
	}

	return true;
}

static void UpdateBlock(World world, SVec* pos, BlockID block) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client client = Clients_List[i];
		if(client && Client_IsInGame(client) && Client_IsInWorld(client, world))
			Packet_WriteSetBlock(client, pos, block);
	}
}

cs_bool Handler_SetBlock(Client client, const char* data) {
	ValidateClientState(client, STATE_INGAME, false);

	World world = client->playerData->world;
	if(!world) return false;
	SVec pos = {0};

	Proto_ReadSVec(&data, &pos);
	cs_uint8 mode = *data++;
	BlockID block = *data;

	switch(mode) {
		case 0x01:
			if(!Block_IsValid(block)) {
				Client_Kick(client, Lang_Get(LANG_KICKBLOCKID));
				return false;
			}
			if(Event_OnBlockPlace(client, mode, &pos, &block)) {
				World_SetBlock(world, &pos, block);
				UpdateBlock(world, &pos, block);
			} else
				Packet_WriteSetBlock(client, &pos, World_GetBlock(world, &pos));
			break;
		case 0x00:
			block = BLOCK_AIR;
			if(Event_OnBlockPlace(client, mode, &pos, &block)) {
				World_SetBlock(world, &pos, block);
				UpdateBlock(world, &pos, block);
			} else
				Packet_WriteSetBlock(client, &pos, World_GetBlock(world, &pos));
			break;
	}

	return true;
}

cs_bool Handler_PosAndOrient(Client client, const char* data) {
	ValidateClientState(client, STATE_INGAME, false);
	CPEData cpd = client->cpeData;
	BlockID cb = *data++;

	if(cpd && cpd->heldBlock != cb) {
		Event_OnHeldBlockChange(client, cpd->heldBlock, cb);
		cpd->heldBlock = cb;
	}

	if(Proto_ReadClientPos(client, data))
		Client_UpdatePositions(client);
	return true;
}

cs_bool Handler_Message(Client client, const char* data) {
	ValidateClientState(client, STATE_INGAME, true);

	MessageType type = 0;
	char message[65];
	char* messptr = message;
	cs_uint8 partial = *data++;
	cs_uint8 len = Proto_ReadStringNoAlloc(&data, message);
	if(len == 0) return false;

	for(cs_uint8 i = 0; i < len; i++) {
		if(message[i] == '%' && ISHEX(message[i + 1]))
			message[i] = '&';
	}

	CPEData cpd = client->cpeData;
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


/*
** Врайтеры и хендлеры
** CPE протокола
*/
#define MODELS_COUNT 15

static const char* validModelNames[MODELS_COUNT] = {
	"humanoid",
	"chicken",
	"creeper",
	"pig",
	"sheep",
	"skeleton",
	"sheep",
	"sheep_nofur",
	"skeleton",
	"spider",
	"zombie",
	"head",
	"sit",
	"chibi",
	NULL
};

cs_bool CPE_CheckModel(cs_int16 model) {
	if(model < 256) return Block_IsValid((BlockID)model);
	return model - 256 < MODELS_COUNT;
}

cs_int16 CPE_GetModelNum(const char* model) {
	cs_int16 modelnum = -1;
	for(cs_int16 i = 0; validModelNames[i]; i++) {
		const char* cmdl = validModelNames[i];
		if(String_CaselessCompare(model, cmdl)) {
			modelnum = i + 256;
			break;
		}
	}
	if(modelnum == -1) {
		cs_int32 tmp = String_ToInt(model);
		if(tmp < 0 || tmp > 255)
			modelnum = 256;
		else
			modelnum = (cs_int16)tmp;
	}

	return modelnum;
}

const char* CPE_GetModelStr(cs_int16 num) {
	return num >= 0 && num < MODELS_COUNT ? validModelNames[num] : NULL;
}

void CPEPacket_WriteInfo(Client client) {
	PacketWriter_Start(client);

	*data++ = 0x10;
	Proto_WriteString(&data, SOFTWARE_FULLNAME);
	*(cs_uint16*)data = htons(extensionsCount);

	PacketWriter_End(client, 67);
}

void CPEPacket_WriteExtEntry(Client client, CPEExt ext) {
	PacketWriter_Start(client);

	*data++ = 0x11;
	Proto_WriteString(&data, ext->name);
	*(cs_uint32*)data = htonl(ext->version);

	PacketWriter_End(client, 69);
}

void CPEPacket_WriteClickDistance(Client client, cs_int16 dist) {
	PacketWriter_Start(client);

	*data = 0x12;
	*(cs_int16*)++data = dist;

	PacketWriter_End(client, 3);
}

// 0x13 - CustomBlocksSupportLevel

void CPEPacket_WriteHoldThis(Client client, BlockID block, cs_bool preventChange) {
	PacketWriter_Start(client);

	*data++ = 0x14;
	*data++ = block;
	*data = (char)preventChange;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteSetHotKey(Client client, const char* action, cs_int32 keycode, cs_int8 keymod) {
	PacketWriter_Start(client);

	*data++ = 0x15;
	Proto_WriteString(&data, NULL); // Label
	Proto_WriteString(&data, action);
	*(cs_int32*)data = htonl(keycode); data += 4;
	*data = keymod;

	PacketWriter_End(client, 134);
}

void CPEPacket_WriteAddName(Client client, Client other) {
	PacketWriter_Start(client);

	*data++ = 0x16;
	data++; // 16 bit id? For what?
	*data++ = client == other ? 0xFF : other->id;
	Proto_WriteString(&data, Client_GetName(other));
	Proto_WriteString(&data, Client_GetName(other));
	CGroup group = Client_GetGroup(other);
	Proto_WriteString(&data, group->name);
	*data = group->rank;

	PacketWriter_End(client, 196);
}

void CPEPacket_WriteAddEntity2(Client client, Client other) {
	PacketWriter_Start(client);

	*data++ = 0x21;
	*data++ = client == other ? 0xFF : other->id;
	Proto_WriteString(&data, Client_GetName(other));
	Proto_WriteString(&data, Client_GetSkin(other));
	cs_bool extended = Client_GetExtVer(client, EXT_ENTPOS) != 0;
	cs_uint32 len = Proto_WriteClientPos(data, other, extended);

	PacketWriter_End(client, 132 + len);
}

void CPEPacket_WriteRemoveName(Client client, Client other) {
	PacketWriter_Start(client);

	*data++ = 0x18;
	data++; // Short value... again.
	*data++ = client == other ? 0xFF : other->id;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteEnvColor(Client client, cs_uint8 type, Color3* col) {
	PacketWriter_Start(client);

	*data++ = 0x19;
	*data++ = type;
	Proto_WriteColor3(&data, col);

	PacketWriter_End(client, 8);
}

void CPEPacket_WriteMakeSelection(Client client, cs_uint8 id, SVec* start, SVec* end, Color4* color) {
	PacketWriter_Start(client);

	*data++ = 0x1A;
	*data++ = id;
	data += 64; // Label
	Proto_WriteSVec(&data, start);
	Proto_WriteSVec(&data, end);
	Proto_WriteColor4(&data, color);

	PacketWriter_End(client, 86);
}

void CPEPacket_WriteRemoveSelection(Client client, cs_uint8 id) {
	PacketWriter_Start(client);

	*data++ = 0x1B;
	*data = id;

	PacketWriter_End(client, 2);
}

void CPEPacket_WriteBlockPerm(Client client, BlockID id, cs_bool allowPlace, cs_bool allowDestroy) {
	PacketWriter_Start(client);

	*data++ = 0x1C;
	*data++ = id;
	*data++ = (char)allowPlace;
	*data = (char)allowDestroy;

	PacketWriter_End(client, 4);
}

void CPEPacket_WriteSetModel(Client client, Client other) {
	PacketWriter_Start(client);

	*data++ = 0x1D;
	*data++ = client == other ? 0xFF : other->id;
	cs_int16 model = Client_GetModel(other);
	if(model < 256) {
		char modelname[4];
		String_FormatBuf(modelname, 4, "%d", model);
		Proto_WriteString(&data, modelname);
	} else
		Proto_WriteString(&data, CPE_GetModelStr(model - 256));

	PacketWriter_End(client, 66);
}

void CPEPacket_WriteWeatherType(Client client, Weather type) {
	PacketWriter_Start(client);

	*data++ = 0x1F;
	*data = type;

	PacketWriter_End(client, 2);
}

void CPEPacket_WriteHackControl(Client client, Hacks hacks) {
	PacketWriter_Start(client);

	*data++ = 0x20;
	*data++ = (char)hacks->flying;
	*data++ = (char)hacks->noclip;
	*data++ = (char)hacks->speeding;
	*data++ = (char)hacks->spawnControl;
	*data++ = (char)hacks->tpv;
	*(cs_int16*)data = hacks->jumpHeight;

	PacketWriter_End(client, 8);
}

// 0x23 - BlockDefinition, 0x24 - RemoveBlockDefinition
// 0x25 - ExtBlockDefinition
// 0x26 - BulkBlockUpdate
// 0x27 - SetTextColor

void CPEPacket_WriteTexturePack(Client client, const char* url) {
	PacketWriter_Start(client);

	*data++ = 0x28;
	Proto_WriteString(&data, url);

	PacketWriter_End(client, 65);
}

void CPEPacket_WriteMapProperty(Client client, cs_uint8 property, cs_int32 value) {
	PacketWriter_Start(client);

	*data++ = 0x29;
	*data++ = property;
	*(int*)data = htonl(value);

	PacketWriter_End(client, 6);
}

// 0x2A - SetEntityProperty

void CPEPacket_WriteTwoWayPing(Client client, cs_uint8 direction, cs_int16 num) {
	PacketWriter_Start(client);

	*data++ = 0x2B;
	*data++ = direction;
	*(cs_uint16*)data = num;

	PacketWriter_End(client, 4);
}

void CPEPacket_WriteInventoryOrder(Client client, Order order, BlockID block) {
	PacketWriter_Start(client);

	*data++ = 0x2C;
	*data++ = block;
	*data = order;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteSetHotBar(Client client, Order order, BlockID block) {
	PacketWriter_Start(client);

	*data++ = 0x2D;
	*data++ = block;
	*data = order;

	PacketWriter_End(client, 3);
}

cs_bool CPEHandler_ExtInfo(Client client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_INITIAL, false);

	if(!Proto_ReadString(&data, &client->cpeData->appName)) return false;
	client->cpeData->_extCount = ntohs(*(cs_uint16*)data);
	return true;
}

cs_bool CPEHandler_ExtEntry(Client client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_INITIAL, false);

	CPEData cpd = client->cpeData;
	CPEExt tmp = Memory_Alloc(1, sizeof(struct cpeExt));
	if(!Proto_ReadString(&data, &tmp->name)) {
		Memory_Free(tmp);
		return false;
	}
	tmp->version = ntohl(*(cs_int32*)data);
	if(tmp->version < 1) {
		Memory_Free(tmp);
		return false;
	}
	tmp->crc32 = String_CRC32((cs_uint8*)tmp->name);

	if(tmp->crc32 == EXT_HACKCTRL && !cpd->hacks)
		cpd->hacks = Memory_Alloc(1, sizeof(struct cpeHacks));
	if(tmp->crc32 == EXT_LONGMSG && !cpd->message)
		cpd->message = Memory_Alloc(1, 193);

	tmp->next = cpd->firstExtension;
	cpd->firstExtension = tmp;

	if(--cpd->_extCount == 0) {
		Event_Call(EVT_ONHANDSHAKEDONE, (void*)client);
		Client_HandshakeStage2(client);
	}

	return true;
}

cs_bool CPEHandler_PlayerClick(Client client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_INGAME, false);

	char button = *data++;
	char action = *data++;
	cs_int16 yaw = ntohs(*(cs_int16*)data); data += 2;
	cs_int16 pitch = ntohs(*(cs_int16*)data); data += 2;
	ClientID tgID = *data++;
	SVec tgBlockPos = {0};
	Proto_ReadSVec(&data, &tgBlockPos);
	char tgBlockFace = *data;

	Event_OnClick(
		client, button,
		action, yaw,
		pitch, tgID,
		&tgBlockPos,
		tgBlockFace
	);
	return true;
}

cs_bool CPEHandler_TwoWayPing(Client client, const char* data) {
	ValidateCpeClient(client, false);
	CPEData cpd = client->cpeData;
	cs_uint8 pingDirection = *data++;
	cs_uint16 pingData = *(cs_uint16*)data;

	if(pingDirection == 0) {
		CPEPacket_WriteTwoWayPing(client, 0, pingData);
		if(!cpd->pingStarted) {
			CPEPacket_WriteTwoWayPing(client, 1, cpd->pingData++);
			cpd->pingStarted = true;
			cpd->pingStart = Time_GetMSec();
		}
		return true;
	} else if(pingDirection == 1) {
		if(cpd->pingStarted) {
			cpd->pingStarted = false;
			if(cpd->pingData == pingData)
				cpd->pingTime = (cs_uint32)((Time_GetMSec() - cpd->pingStart) / 2);
			return true;
		}
	}
	return false;
}
