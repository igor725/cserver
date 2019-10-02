#include "core.h"
#include "block.h"
#include "client.h"
#include "cpe.h"
#include "packets.h"
#include "event.h"

EXT* headExtension;
uint16_t extensionsCount;

void CPE_RegisterExtension(const char* name, int version) {
	EXT* tmp = Memory_Alloc(1, sizeof(EXT));

	tmp->name = name;
	tmp->version = version;
	tmp->next = headExtension;
	headExtension = tmp;
	++extensionsCount;
}

void CPE_StartHandshake(CLIENT client) {
	CPEPacket_WriteInfo(client);
	EXT* ptr = headExtension;
	while(ptr) {
		CPEPacket_WriteExtEntry(client, ptr);
		ptr = ptr->next;
	}
}

static const struct extReg serverExtensions[] = {
	{"FastMap", 1},
	{"EmoteFix", 1},
	{"FullCP437", 1},
	{"SetHotbar", 1},
	{"HeldBlock", 1},
	{"TwoWayPing", 1},
	{"PlayerClick", 1},
	{"ChangeModel", 1},
	{"MessageTypes", 1},
	{"ClickDistance", 1},
	{"InventoryOrder", 1},
	{"EnvWeatherType", 1},
	{"BlockPermissions", 1},
	{NULL, 0}
};

static const char* validModelNames[] = {
	"chicken",
	"creeper",
	"humanoid",
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

bool CPE_CheckModel(const char* model) {
	for(int i = 0; validModelNames[i]; i++) {
		const char* cmdl = validModelNames[i];
		if(String_CaselessCompare(model, cmdl)) {
			return true;
		}
	}

	BlockID block;
	if((block = (BlockID)String_ToInt(model)) > 0) {
		return Block_IsValid(block);
	}

	return false;
}

void Packet_RegisterCPEDefault(void) {
	const struct extReg* ext;
	for(ext = serverExtensions; ext->name; ext++) {
		CPE_RegisterExtension(ext->name, ext->version);
	}
	Packet_Register(0x10, "ExtInfo", 67, CPEHandler_ExtInfo);
	Packet_Register(0x11, "ExtEntry", 69, CPEHandler_ExtEntry);
	Packet_Register(0x2B, "TwoWayPing", 4, CPEHandler_TwoWayPing);
	Packet_Register(0x22, "PlayerClick", 15, CPEHandler_PlayerClick);
}

void CPEPacket_WriteInfo(CLIENT client) {
	PacketWriter_Start(client);

	*data = 0x10;
	WriteNetString(++data, SOFTWARE_NAME " " SOFTWARE_VERSION); data += 63;
	*(uint16_t*)++data = htons(extensionsCount);

	PacketWriter_End(client, 67);
}

void CPEPacket_WriteExtEntry(CLIENT client, EXT* ext) {
	PacketWriter_Start(client);

	*data = 0x11;
	WriteNetString(++data, ext->name); data += 63;
	*(uint32_t*)++data = htonl(ext->version);

	PacketWriter_End(client, 69);
}

void CPEPacket_WriteClickDistance(CLIENT client, short dist) {
	PacketWriter_Start(client);

	*data = 0x12;
	*(short*)++data = dist;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteInventoryOrder(CLIENT client, Order order, BlockID block) {
	PacketWriter_Start(client);

	*data = 0x44;
	*++data = order;
	*++data = block;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteHoldThis(CLIENT client, BlockID block, bool preventChange) {
	PacketWriter_Start(client);

	*data = 0x14;
	*++data = block;
	*++data = (char)preventChange;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteSetHotBar(CLIENT client, Order order, BlockID block) {
	PacketWriter_Start(client);

	*data = 0x2D;
	*++data = block;
	*++data = order;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteWeatherType(CLIENT client, Weather type) {
	PacketWriter_Start(client);

	*data = 0x1F;
	*++data = type;

	PacketWriter_End(client, 2);
}

void CPEPacket_WriteTwoWayPing(CLIENT client, uint8_t direction, short num) {
	PacketWriter_Start(client);
	if(client->playerData->state != STATE_INGAME) {
		PacketWriter_Stop(client);
	}

	*data = 0x2B;
	*++data = direction;
	*(uint16_t*)++data = num;

	PacketWriter_End(client, 4);
}

void CPEPacket_WriteSetModel(CLIENT client, ClientID id, const char* model) {
	PacketWriter_Start(client);

	*data = 0x01D;
	*++data = id;
	WriteNetString(++data, model);

	PacketWriter_End(client, 66);
}

void CPEPacket_WriteBlockPerm(CLIENT client, BlockID id, bool allowPlace, bool allowDestroy) {
	PacketWriter_Start(client);

	*data = 0x1C;
	*++data = id;
	*++data = (char)allowPlace;
	*++data = (char)allowDestroy;

	PacketWriter_End(client, 4);
}

/*
	CPE packet handlers
*/

#define ValidateClientState(client, st) \
if(!client->playerData || !client->cpeData || client->playerData->state != st) \
	return false; \

bool CPEHandler_ExtInfo(CLIENT client, char* data) {
	ValidateClientState(client, STATE_MOTD);

	ReadNetString(data, &client->cpeData->appName); data += 63;
	client->cpeData->_extCount = ntohs(*(uint16_t*)++data);
	return true;
}

bool CPEHandler_ExtEntry(CLIENT client, char* data) {
	ValidateClientState(client, STATE_MOTD);

	EXT* tmp = Memory_Alloc(1, sizeof(EXT));
	ReadNetString(data, (void*)&tmp->name);data += 63;
	tmp->version = ntohl(*(uint32_t*)++data);

	if(String_CaselessCompare(tmp->name, "FastMap"))
		client->cpeData->fmSupport = true;

	tmp->next = client->cpeData->headExtension;
	client->cpeData->headExtension = tmp;

	--client->cpeData->_extCount;
	if(client->cpeData->_extCount == 0) {
		Event_Call(EVT_ONHANDSHAKEDONE, (void*)client);
		Client_HandshakeStage2(client);
	}

	return true;
}

bool CPEHandler_TwoWayPing(CLIENT client, char* data) {
	if(*data == 0) {
		CPEPacket_WriteTwoWayPing(client, 0, *(uint16_t*)++data);
		return true;
	} else if(*data == 1) {
		// TODO: Обрабатывать ответ от клиента на пинг пакет
		return true;
	}
	return false;
}

bool CPEHandler_PlayerClick(CLIENT client, char* data) {
	ValidateClientState(client, STATE_INGAME);
	char button = *data;
	char action = *++data;
	short yaw = ntohs(*(uint16_t*)++data); ++data;
	short pitch = ntohs(*(uint16_t*)++data); ++data;
	ClientID tgID = *++data;
	short tgBlockX = ntohs(*(short*)++data); ++data;
	short tgBlockY = ntohs(*(short*)++data); ++data;
	short tgBlockZ = ntohs(*(short*)++data); ++data;
	char tgBlockFace = *++data;

	Event_OnClick(
		client, &button,
		&action, &yaw,
		&pitch, &tgID,
		&tgBlockX,
		&tgBlockY,
		&tgBlockZ,
		&tgBlockFace
	);
	return true;
}
