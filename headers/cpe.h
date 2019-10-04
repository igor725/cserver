#ifndef CPE_H
#define CPE_H

#define ValidateCpeClient(client, ret) \
if(!client->cpeData) return ret;

struct extReg {
	const char* name;
	int version;
};

API bool CPE_CheckModel(const char* model);
void CPE_StartHandshake(CLIENT client);

bool CPEHandler_ExtInfo(CLIENT client, char* data);
bool CPEHandler_ExtEntry(CLIENT client, char* data);
bool CPEHandler_TwoWayPing(CLIENT client, char* data);
bool CPEHandler_PlayerClick(CLIENT client, char* data);
bool CPEHandler_PosAndOrient(CLIENT client, char* data);

void CPEPacket_WriteInfo(CLIENT client);
void CPEPacket_WriteExtEntry(CLIENT client, EXT ext);
void CPEPacket_WriteClickDistance(CLIENT client, short dist);
void CPEPacket_WriteInventoryOrder(CLIENT client, Order order, BlockID block);
void CPEPacket_WriteHoldThis(CLIENT client, BlockID block, bool preventChange);
void CPEPacket_WriteSetHotBar(CLIENT client, Order order, BlockID block);
void CPEPacket_WriteWeatherType(CLIENT client, Weather type);
void CPEPacket_WriteTexturePack(CLIENT client, const char* url);
void CPEPacket_WriteMapProperty(CLIENT client, uint8_t property, int value);
void CPEPacket_WriteTwoWayPing(CLIENT client, uint8_t direction, short num);
void CPEPacket_WriteSetModel(CLIENT client, ClientID id, const char* model);
void CPEPacket_WriteBlockPerm(CLIENT client, BlockID id, bool allowPlace, bool allowDestroy);
#endif
