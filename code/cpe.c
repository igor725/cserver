#include "core.h"
#include "platform.h"
#include "str.h"
#include "block.h"
#include "client.h"
#include "cpe.h"
#include "packets.h"
#include "event.h"

CPEExt firstExtension;
uint16_t extensionsCount;

void CPE_RegisterExtension(const char* name, int32_t version) {
	CPEExt tmp = Memory_Alloc(1, sizeof(struct cpeExt));

	tmp->name = name;
	tmp->version = version;
	tmp->next = firstExtension;
	firstExtension = tmp;
	++extensionsCount;
}

void CPE_StartHandshake(Client client) {
	CPEPacket_WriteInfo(client);
	CPEExt ptr = firstExtension;
	while(ptr) {
		CPEPacket_WriteExtEntry(client, ptr);
		ptr = ptr->next;
	}
}

struct extReg {
	const char* name;
	int32_t version;
};

static const struct extReg serverExtensions[] = {
	{"ClickDistance", 1},
	// CustomBlocks
	{"HeldBlock", 1},
	{"EmoteFix", 1},
	// TextHotKey
	// ExtPlayerList
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

bool CPE_CheckModel(int16_t model) {
	if(model < 256) return Block_IsValid((BlockID)model);
	return model - 256 < MODELS_COUNT;
}

int16_t CPE_GetModelNum(const char* model) {
	int16_t modelnum = -1;
	for(int16_t i = 0; validModelNames[i]; i++) {
		const char* cmdl = validModelNames[i];
		if(String_CaselessCompare(model, cmdl)) {
			modelnum = i + 256;
			break;
		}
	}
	if(modelnum == -1) {
		int32_t tmp = String_ToInt(model);
		if(tmp < 0 || tmp > 255)
			modelnum = 256;
		else
			modelnum = (int16_t)tmp;
	}

	return modelnum;
}

const char* CPE_GetModelStr(int16_t num) {
	return num >= 0 && num < MODELS_COUNT ? validModelNames[num] : NULL;
}

void Packet_RegisterCPEDefault(void) {
	const struct extReg* ext;
	for(ext = serverExtensions; ext->name; ext++) {
		CPE_RegisterExtension(ext->name, ext->version);
	}
	Packet_Register(0x10, "ExtInfo", 66, CPEHandler_ExtInfo);
	Packet_Register(0x11, "ExtEntry", 68, CPEHandler_ExtEntry);
	Packet_Register(0x2B, "TwoWayPing", 3, CPEHandler_TwoWayPing);
	Packet_Register(0x22, "PlayerClick", 14, CPEHandler_PlayerClick);
	Packet_RegisterCPE(0x08, EXT_ENTPOS, 1, 15, NULL);
}

void CPEPacket_WriteInfo(Client client) {
	PacketWriter_Start(client);

	*data++ = 0x10;
	Proto_WriteString(&data, SOFTWARE_FULLNAME);
	*(uint16_t*)data = htons(extensionsCount);

	PacketWriter_End(client, 67);
}

void CPEPacket_WriteExtEntry(Client client, CPEExt ext) {
	PacketWriter_Start(client);

	*data++ = 0x11;
	Proto_WriteString(&data, ext->name);
	*(uint32_t*)data = htonl(ext->version);

	PacketWriter_End(client, 69);
}

void CPEPacket_WriteClickDistance(Client client, short dist) {
	PacketWriter_Start(client);

	*data = 0x12;
	*(short*)++data = dist;

	PacketWriter_End(client, 3);
}

// 0x13 - CustomBlocksSupportLevel

void CPEPacket_WriteHoldThis(Client client, BlockID block, bool preventChange) {
	PacketWriter_Start(client);

	*data++ = 0x14;
	*data++ = block;
	*data = (char)preventChange;

	PacketWriter_End(client, 3);
}

// 0x15 - SetTextHotKey

void CPEPacket_WriteEnvColor(Client client, uint8_t type, Color3* col) {
	PacketWriter_Start(client);

	*data++ = 0x19;
	*data++ = type;
	Proto_WriteColor3(&data, col);

	PacketWriter_End(client, 8);
}

void CPEPacket_WriteMakeSelection(Client client, uint8_t id, SVec* start, SVec* end, Color4* color) {
	PacketWriter_Start(client);

	*data++ = 0x1A;
	*data++ = id;
	data += 64; // Label
	Proto_WriteSVec(&data, start);
	Proto_WriteSVec(&data, end);
	Proto_WriteColor4(&data, color);

	PacketWriter_End(client, 86);
}

void CPEPacket_WriteRemoveSelection(Client client, uint8_t id) {
	PacketWriter_Start(client);

	*data++ = 0x1B;
	*data = id;

	PacketWriter_End(client, 2);
}

void CPEPacket_WriteBlockPerm(Client client, BlockID id, bool allowPlace, bool allowDestroy) {
	PacketWriter_Start(client);

	*data++ = 0x1C;
	*data++ = id;
	*data++ = (char)allowPlace;
	*data = (char)allowDestroy;

	PacketWriter_End(client, 4);
}

void CPEPacket_WriteSetModel(Client client, ClientID id, int16_t model) {
	PacketWriter_Start(client);

	*data++ = 0x1D;
	*data++ = id;
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
	*(short*)data = hacks->jumpHeight;

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

void CPEPacket_WriteMapProperty(Client client, uint8_t property, int32_t value) {
	PacketWriter_Start(client);

	*data++ = 0x29;
	*data++ = property;
	*(int*)data = htonl(value);

	PacketWriter_End(client, 6);
}

// 0x2A - SetEntityProperty

void CPEPacket_WriteTwoWayPing(Client client, uint8_t direction, short num) {
	PacketWriter_Start(client);
	if(client->playerData->state != STATE_INGAME) {
		PacketWriter_Stop(client);
	}

	*data++ = 0x2B;
	*data++ = direction;
	*(uint16_t*)data = num;

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

bool CPEHandler_ExtInfo(Client client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_MOTD, false);

	if(!Proto_ReadString(&data, &client->cpeData->appName)) return false;
	client->cpeData->_extCount = ntohs(*(uint16_t*)data);
	return true;
}

bool CPEHandler_ExtEntry(Client client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_MOTD, false);

	CPEData cpd = client->cpeData;
	CPEExt tmp = Memory_Alloc(1, sizeof(struct cpeExt));
	if(!Proto_ReadString(&data, &tmp->name)) {
		Memory_Free(tmp);
		return false;
	}
	tmp->version = ntohl(*(int32_t*)data);
	if(tmp->version < 1) {
		Memory_Free(tmp);
		return false;
	}
	tmp->crc32 = String_CRC32((uint8_t*)tmp->name);

	if(tmp->crc32 == EXT_HACKCTRL && !cpd->hacks)
		cpd->hacks = Memory_Alloc(1, sizeof(struct cpeHacks));
	if(tmp->crc32 == EXT_LONGMSG && !cpd->message)
		cpd->message = Memory_Alloc(1, 193);

	tmp->next = cpd->firstExtension;
	cpd->firstExtension = tmp;

	--cpd->_extCount;
	if(cpd->_extCount == 0) {
		Event_Call(EVT_ONHANDSHAKEDONE, (void*)client);
		Client_HandshakeStage2(client);
	}

	return true;
}

bool CPEHandler_PlayerClick(Client client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_INGAME, false);

	char button = *data++;
	char action = *data++;
	short yaw = ntohs(*(uint16_t*)data); data += 2;
	short pitch = ntohs(*(uint16_t*)data); data += 2;
	ClientID tgID = *data++;
	SVec tgBlockPos = {0};
	tgBlockPos.x = ntohs(*(short*)data); data += 2;
	tgBlockPos.y = ntohs(*(short*)data); data += 2;
	tgBlockPos.z = ntohs(*(short*)data); data += 2;
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

bool CPEHandler_TwoWayPing(Client client, const char* data) {
	ValidateCpeClient(client, false);
	CPEData cpd = client->cpeData;
	uint8_t pingDirection = *data++;
	uint16_t pingData = *(uint16_t*)data;

	if(pingDirection == 0) {
		CPEPacket_WriteTwoWayPing(client, 0, pingData);
		CPEPacket_WriteTwoWayPing(client, 1, cpd->pingData++);
		cpd->pingStarted = true;
		cpd->pingStart = Time_GetMSec();
		return true;
	} else if(pingDirection == 1) {
		if(cpd->pingStarted) {
			cpd->pingStarted = false;
			if(cpd->pingData == pingData)
				cpd->pingTime = (uint32_t)((Time_GetMSec() - cpd->pingStart) / 2);
			return true;
		}
	}
	return false;
}
