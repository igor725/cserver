#ifndef CPE_H
#define CPE_H
#define EXT_CLICKDIST 0x6DD2B567ul
#define EXT_CUSTOMBLOCKS 0x98455F43ul
#define EXT_HELDBLOCK 0x40C33F88ul
#define EXT_EMOTEFIX 0x89C01AE6ul
#define EXT_TEXTHOTKEY 0x73BB9FBFul
#define EXT_PLAYERLIST 0xBB0CD618ul
#define EXT_ENVCOLOR 0x4C056274ul
#define EXT_CUBOID 0xE45DA299ul
#define EXT_BLOCKPERM 0xB2E8C3D6ul
#define EXT_CHANGEMODEL 0xAE3AEBAAul
#define EXT_MAPPROPS 0xB46CAFABul
#define EXT_WEATHER 0x40501770ul
#define EXT_MESSAGETYPE 0x7470960Eul
#define EXT_HACKCTRL 0x6E4CED2Dul
#define EXT_PLAYERCLICK 0x29442DBul
#define EXT_CP437 0x27FBB82Ful
#define EXT_LONGMSG 0x8535AB13ul
#define EXT_BLOCKDEF 0xC6BAA7Bul
#define EXT_BLOCKDEF2 0xEFB2BBECul
#define EXT_BULKUPDATE 0x29509B8Ful
#define EXT_EXTCLRS 0x56C393B8ul
#define EXT_MAPASPECT 0xB3F9BDF0ul
#define EXT_ENTPROP 0x5865D50Eul
#define EXT_ENTPOS 0x37D3033Ful
#define EXT_TWOWAYPING 0xBBC796E8ul
#define EXT_INVORDER 0xEE0F7B71ul
#define EXT_INSTANTMOTD 0x462BFA8Ful
#define EXT_FASTMAP 0x7791DB5Ful
#define EXT_SETHOTBAR 0xB8703914ul
#define EXT_MORETEXTURES 0xBFAA6298ul
#define EXT_MOREBLOCKS 0xA349DCECul

#define ValidateCpeClient(client, ret) \
if(!client->cpeData) return ret;

API cs_bool CPE_CheckModel(cs_int16 model);
API void CPE_RegisterExtension(const char* name, cs_int32 version);
API cs_int16 CPE_GetModelNum(const char* model);
API const char* CPE_GetModelStr(cs_int16 num);
void CPE_StartHandshake(Client client);

cs_bool CPEHandler_ExtInfo(Client client, const char* data);
cs_bool CPEHandler_ExtEntry(Client client, const char* data);
cs_bool CPEHandler_TwoWayPing(Client client, const char* data);
cs_bool CPEHandler_PlayerClick(Client client, const char* data);

void CPEPacket_WriteInfo(Client client);
void CPEPacket_WriteExtEntry(Client client, CPEExt ext);
void CPEPacket_WriteClickDistance(Client client, cs_int16 dist);
void CPEPacket_WriteInventoryOrder(Client client, Order order, BlockID block);
void CPEPacket_WriteHoldThis(Client client, BlockID block, cs_bool preventChange);
void CPEPacket_WriteAddName(Client client, Client other);
void CPEPacket_WriteAddEntity2(Client client, Client other);
void CPEPacket_WriteRemoveName(Client client, Client other);
void CPEPacket_WriteEnvColor(Client client, cs_uint8 type, Color3* col);
void CPEPacket_WriteMakeSelection(Client client, cs_uint8 id, SVec* start, SVec* end, Color4* color);
void CPEPacket_WriteRemoveSelection(Client client, cs_uint8 id);
void CPEPacket_WriteHackControl(Client client, Hacks hacks);
void CPEPacket_WriteSetHotBar(Client client, Order order, BlockID block);
void CPEPacket_WriteWeatherType(Client client, Weather type);
void CPEPacket_WriteTexturePack(Client client, const char* url);
void CPEPacket_WriteMapProperty(Client client, cs_uint8 property, cs_int32 value);
void CPEPacket_WriteTwoWayPing(Client client, cs_uint8 direction, cs_int16 num);
void CPEPacket_WriteSetModel(Client client, ClientID id, cs_int16 model);
void CPEPacket_WriteBlockPerm(Client client, BlockID id, cs_bool allowPlace, cs_bool allowDestroy);
#endif
