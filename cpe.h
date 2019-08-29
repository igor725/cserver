#ifndef CPE_H
#define CPE_H
typedef unsigned char Weather;
typedef unsigned char MessageType;

enum WeatherTypes {
	WEATHER_SUN = 0,
	WEATHER_RAIN,
	WEATHER_SNOW
};

struct extReg {
	const char* name;
	int version;
};

void CPEPacket_WriteInfo(CLIENT *cl);
void CPE_StartHandshake(CLIENT* client);
bool CPEHandler_ExtInfo(CLIENT* client, char* data);
bool CPEHandler_ExtEntry(CLIENT* client, char* data);
bool CPEHandler_TwoWayPing(CLIENT* client, char* data);
bool CPEHandler_PlayerClick(CLIENT* client, char* data);

void CPEPacket_WriteInfo(CLIENT *client);
void CPEPacket_WriteExtEntry(CLIENT* client, EXT* ext);
void CPEPAcket_WriteClickDistance(CLIENT* client, short dist);
void CPEPacket_WriteWeatherType(CLIENT* client, Weather type);
void CPEPacket_WriteSetHotBar(CLIENT* client, Order order, BlockID block);
void CPEPacket_WriteInventoryOrder(CLIENT* client, Order order, BlockID block);
void CPEPacket_WriteHoldThis(CLIENT* client, BlockID block, bool preventChange);

EXT* headExtension;
ushort extensionsCount;
#endif
