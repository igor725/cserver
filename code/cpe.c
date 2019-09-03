#include "core.h"
#include "client.h"
#include "cpe.h"
#include "packets.h"
#include "event.h"

void CPE_RegisterExtension(const char* name, int version) {
	EXT* tmp = (EXT*)Memory_Alloc(1, sizeof(EXT));

	tmp->name = name;
	tmp->version = version;
	tmp->next = headExtension;
	headExtension = tmp;
	++extensionsCount;
}

void CPE_StartHandshake(CLIENT* client) {
	CPEPacket_WriteInfo(client);
	EXT* ptr = headExtension;
	while(ptr) {
		CPEPacket_WriteExtEntry(client, ptr);
		ptr = ptr->next;
	}
}

static const struct extReg serverExtensions[] = {
	{"FastMap", 1},
	{"SetHotbar", 1},
	{"HeldBlock", 1},
	{"TwoWayPing", 1},
	// {"PlayerClick", 1},
	{"ChangeModel", 1},
	{"MessageTypes", 1},
	{"ClickDistance", 1},
	{"InventoryOrder", 1},
	{"EnvWeatherType", 1},
	{"BlockPermissions", 1},
	{NULL, 0}
};

void Packet_RegisterCPEDefault() {
	const struct extReg* ext;
	for(ext = serverExtensions; ext->name; ext++) {
		CPE_RegisterExtension(ext->name, ext->version);
	}
	Packet_Register(0x10, "ExtInfo", 67, &CPEHandler_ExtInfo);
	Packet_Register(0x11, "ExtEntry", 69, &CPEHandler_ExtEntry);
	Packet_Register(0x2B, "TwoWayPing", 4, &CPEHandler_TwoWayPing);
	Packet_Register(0x22, "PlayerClick", 15, &CPEHandler_PlayerClick);
}

void CPEPacket_WriteInfo(CLIENT* client) {
	char* data = client->wrbuf;
	*data = 0x10;
	WriteString(++data, SOFTWARE_NAME " " SOFTWARE_VERSION); data += 63;
	*(ushort*)++data = htons(extensionsCount);
	Client_Send(client, 67);
}

void CPEPacket_WriteExtEntry(CLIENT* client, EXT* ext) {
	char* data = client->wrbuf;
	*data = 0x11;
	WriteString(++data, ext->name); data += 63;
	*(uint*)++data = htonl(ext->version);
	Client_Send(client, 69);
}

void CPEPAcket_WriteClickDistance(CLIENT* client, short dist) {
	char* data = client->wrbuf;
	*data = 0x12;
	*(short*)++data = dist;
	Client_Send(client, 3);
}

void CPEPacket_WriteInventoryOrder(CLIENT* client, Order order, BlockID block) {
	if(!Block_IsValid(block))
		return;

	char* data = client->wrbuf;
	*data = 0x44;
	*++data = order;
	*++data = block;
	Client_Send(client, 3);
}

void CPEPacket_WriteHoldThis(CLIENT* client, BlockID block, bool preventChange) {
	if(!Block_IsValid(block))
		return;

	char* data = client->wrbuf;
	*data = 0x14;
	*++data = block;
	*++data = (char)preventChange;
	Client_Send(client, 3);
}

void CPEPacket_WriteSetHotBar(CLIENT* client, Order order, BlockID block) {
	if(!Block_IsValid(block))
		return;

	char* data = client->wrbuf;
	*data = 0x2D;
	*++data = block;
	*++data = order;
	Client_Send(client, 3);
}

void CPEPacket_WriteWeatherType(CLIENT* client, Weather type) {
	char* data = client->wrbuf;
	*data = 0x1F;
	*++data = type;
	Client_Send(client, 2);
}

void CPEPacket_WriteTwoWayPing(CLIENT* client, uchar direction, short num) {
	char* data = client->wrbuf;
	*data = 0x2B;
	*++data = direction;
	*(ushort*)++data = num;
	Client_Send(client, 4);
}

void CPEPACKET_WriteSetModel(CLIENT* client, ClientID id, const char* model) {
	char* data = client->wrbuf;
	*data = 0x01D;
	*++data = id;
	WriteString(++data, model);
	Client_Send(client, 66);
}

void CPEPacket_WriteBlockPerm(CLIENT* client, BlockID id, bool allowPlace, bool allowDestroy) {
	char* data = client->wrbuf;
	*data = 0x1C;
	*++data = id;
	*++data = (char)allowPlace;
	*++data = (char)allowDestroy;
	Client_Send(client, 4);
}

bool CPEHandler_ExtInfo(CLIENT* client, char* data) {
	if(!client->playerData || !client->cpeData || client->playerData->state != STATE_MOTD)
		return false;

	ReadString(data, &client->cpeData->appName); data += 63;
	client->cpeData->_extCount = ntohs(*(ushort*)++data);
	return true;
}

bool CPEHandler_ExtEntry(CLIENT* client, char* data) {
	if(!client->playerData || !client->cpeData || client->playerData->state != STATE_MOTD)
		return false;

	EXT* tmp = (EXT*)Memory_Alloc(1, sizeof(EXT));
	ReadString(data, (void*)&tmp->name);data += 63;
	tmp->version = ntohl(*(uint*)++data);

	if(String_CaselessCompare(tmp->name, "FastMap")) {
		client->cpeData->fmSupport = true;
	}

	tmp->next = client->cpeData->headExtension;
	client->cpeData->headExtension = tmp;

	--client->cpeData->_extCount;
	if(client->cpeData->_extCount == 0) {
		Event_OnHandshakeDone(client);
		Client_HandshakeStage2(client);
	}

	return true;
}

bool CPEHandler_TwoWayPing(CLIENT* client, char* data) {
	if(*data == 0) {
		/*
			Возможно ли тут перемешивание отправляемых данных клиенту?
			По факту функция вызывается из потока с данными от клиента,
			а не из основного, соответственно, если они одновременно
			будут отправлять пакет клиенту, то произойдёт троллинг.
			Но вызвать повреждение подготовленных пакетов у меня
			не получилось ¯\_(ツ)_/¯

			P.S. Стоит всё же предостеречься и реализовать позже
			что-то типа мьютексов.
		*/
		CPEPacket_WriteTwoWayPing(client, 0,* (ushort*)++data);
		return true;
	} else if(*data == 1) {
		// TODO: Обрабатывать ответ от клиента на пинг пакет
		return true;
	}
	return false;
}

bool CPEHandler_PlayerClick(CLIENT* client, char* data) {
	if(!client->playerData || !client->cpeData || client->playerData->state != STATE_INGAME)
		return false;

	return true;
}
