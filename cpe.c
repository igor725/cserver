#include "core.h"
#include "client.h"
#include "cpe.h"
#include "packets.h"
#include "event.h"
#include "block.h"

void CPE_RegisterExtension(char* name, int version) {
	EXT* tmp = calloc(1, sizeof(struct ext));

	tmp->name = name;
	tmp->version = version;

	if(!tailExtension) {
		firstExtension = tmp;
		tailExtension = tmp;
	} else {
		tailExtension->next = tmp;
		tailExtension = tmp;
	}
	++extensionsCount;
}

void CPE_StartHandshake(CLIENT* client) {
	CPEPacket_WriteInfo(client);
	EXT* ptr = firstExtension;
	while(ptr != NULL) {
		CPEPacket_WriteExtEntry(client, ptr);
		ptr = ptr->next;
	}
}

void Packet_RegisterCPEDefault() {
	CPE_RegisterExtension("SetHotbar", 1);
	CPE_RegisterExtension("HeldBlock", 1);
	CPE_RegisterExtension("MessageTypes", 1);
	CPE_RegisterExtension("InventoryOrder", 1);
	Packet_Register(0x10, "ExtInfo", 67, &CPEHandler_ExtInfo);
	Packet_Register(0x11, "ExtEntry", 69, &CPEHandler_ExtEntry);
}

void CPEPacket_WriteInfo(CLIENT *client) {
	char* data = client->wrbuf;
	*data = 0x10;
	WriteString(++data, SOFTWARE_NAME DELIM SOFTWARE_VERSION); data += 63;
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
	*++data = preventChange;
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

bool CPEHandler_ExtInfo(CLIENT* client, char* data) {
	if(client->cpeData == NULL) return false;

	ReadString(data, &client->cpeData->appName); data += 63;
	client->cpeData->_extCount = ntohs(*(ushort*)++data);
	return true;
}

bool CPEHandler_ExtEntry(CLIENT* client, char* data) {
	if(client->cpeData == NULL) return false;

	EXT* tmp = calloc(1, sizeof(struct ext));

	ReadString(data, &tmp->name);data += 63;
	tmp->version = ntohl(*(uint*)++data);

	if(client->cpeData->tailExtension == NULL) {
		client->cpeData->firstExtension = tmp;
		client->cpeData->tailExtension = tmp;
	} else {
		client->cpeData->tailExtension->next = tmp;
		client->cpeData->tailExtension = tmp;
	}

	--client->cpeData->_extCount;
	if(client->cpeData->_extCount == 0) {
		Event_OnHandshakeDone(client);
		if(!Client_SendMap(client))
			Client_Kick(client, "Map sending failed");
	}

	return true;
}
