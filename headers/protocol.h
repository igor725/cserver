#ifndef PROTOCOL_H
#define PROTOCOL_H
#define ValidateClientState(client, st, ret) \
if(!client->playerData || client->playerData->state != st) \
	return ret;

#define ValidateCpeClient(client, ret) \
if(!client->cpeData) return ret;

#define PacketWriter_Start(client) \
if(client->closed) return; \
char* data = client->wrbuf; \
Mutex_Lock(client->mutex);

#define PacketWriter_End(client, size) \
Client_Send(client, size); \
Mutex_Unlock(client->mutex);

#define PacketWriter_Stop(client) \
Mutex_Unlock(client->mutex); \
return;

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
#define EXT_TEXTCOLORS 0x56C393B8ul
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
#define EXT_VELCTRL 0xF8DF4FF7

typedef cs_bool(*packetHandler)(Client, const char*);

typedef struct packet {
	cs_uint16 size;
	cs_bool haveCPEImp;
	cs_uint32 extCRC32;
	cs_int32 extVersion;
	cs_uint16 extSize;
	packetHandler handler;
	packetHandler cpeHandler;
} *Packet;

Packet Packet_Get(cs_int32 id);
API void Packet_Register(cs_int32 id, cs_uint16 size, packetHandler handler);
API void Packet_RegisterCPE(cs_int32 id, cs_uint32 extCRC32, cs_int32 extVersion, cs_uint16 extSize, packetHandler handler);

API cs_uint8 Proto_ReadString(const char** data, const char** dst);
API cs_uint8 Proto_ReadStringNoAlloc(const char** data, char* dst);
API void Proto_ReadSVec(const char** dataptr, SVec* vec);
API void Proto_ReadAng(const char** dataptr, Ang* ang);
API void Proto_ReadFlSVec(const char** dataptr, Vec* vec);
API void Proto_ReadFlVec(const char** dataptr, Vec* vec);
API cs_bool Proto_ReadClientPos(Client client, const char* data);

API void Proto_WriteString(char** dataptr, const char* string);
API void Proto_WriteFlVec(char** dataptr, const Vec* vec);
API void Proto_WriteFlSVec(char** dataptr, const Vec* vec);
API void Proto_WriteSVec(char** dataptr, const SVec* vec);
API void Proto_WriteAng(char** dataptr, const Ang* ang);
API void Proto_WriteColor3(char** dataptr, const Color3* color);
API void Proto_WriteColor4(char** dataptr, const Color4* color);
API void Proto_WriteByteColor3(char** dataptr, const Color3* color);
API void Proto_WriteByteColor4(char** dataptr, const Color4* color);
API cs_uint32 Proto_WriteClientPos(char* data, Client client, cs_bool extended);

void Packet_RegisterDefault(void);

/*
** Врайтеры и хендлеры
** ванильного протокола
*/

void Vanilla_WriteHandshake(Client client, const char* name, const char* motd);
void Vanilla_WriteLvlInit(Client client);
void Vanilla_WriteLvlFin(Client client, SVec* dims);
void Vanilla_WriteSetBlock(Client client, SVec* pos, BlockID block);
void Vanilla_WriteSpawn(Client client, Client other);
void Vanilla_WritePosAndOrient(Client client, Client other);
void Vanilla_WriteDespawn(Client client, Client other);
void Vanilla_WriteChat(Client client, MessageType type, const char* mesg);
void Vanilla_WriteKick(Client client, const char* reason);

cs_bool Handler_Handshake(Client client, const char* data);
cs_bool Handler_SetBlock(Client client, const char* data);
cs_bool Handler_PosAndOrient(Client client, const char* data);
cs_bool Handler_Message(Client client, const char* data);

/*
** Врайтеры и хендлеры
** CPE протокола и прочие,
** связанные с CPE вещи
*/
API cs_bool CPE_CheckModel(cs_int16 model);
API void CPE_RegisterExtension(const char* name, cs_int32 version);
API cs_int16 CPE_GetModelNum(const char* model);
API const char* CPE_GetModelStr(cs_int16 num);

cs_bool CPEHandler_ExtInfo(Client client, const char* data);
cs_bool CPEHandler_ExtEntry(Client client, const char* data);
cs_bool CPEHandler_TwoWayPing(Client client, const char* data);
cs_bool CPEHandler_PlayerClick(Client client, const char* data);

void CPE_WriteInfo(Client client);
void CPE_WriteExtEntry(Client client, CPEExt ext);
void CPE_WriteClickDistance(Client client, cs_int16 dist);
void CPE_WriteInventoryOrder(Client client, Order order, BlockID block);
void CPE_WriteHoldThis(Client client, BlockID block, cs_bool preventChange);
void CPE_WriteSetHotKey(Client client, const char* action, cs_int32 keycode, cs_int8 keymod);
void CPE_WriteAddName(Client client, Client other);
void CPE_WriteAddEntity2(Client client, Client other);
void CPE_WriteRemoveName(Client client, Client other);
void CPE_WriteEnvColor(Client client, cs_uint8 type, Color3* col);
void CPE_WriteMakeSelection(Client client, cs_uint8 id, SVec* start, SVec* end, Color4* color);
void CPE_WriteRemoveSelection(Client client, cs_uint8 id);
void CPE_WriteHackControl(Client client, Hacks hacks);
void CPE_WriteDefineBlock(Client client, BlockDef block);
void CPE_WriteUndefineBlock(Client client, BlockID id);
void CPE_WriteDefineExBlock(Client client, BlockDef block);
void CPE_WriteBulkBlockUpdate(Client client, BulkBlockUpdate bbu);
void CPE_WriteSetTextColor(Client client, Color4* color, char code);
void CPE_WriteSetHotBar(Client client, Order order, BlockID block);
void CPE_WriteVelocityControl(Client client, Vec* velocity, cs_bool mode);
void CPE_WriteWeatherType(Client client, Weather type);
void CPE_WriteTexturePack(Client client, const char* url);
void CPE_WriteMapProperty(Client client, cs_uint8 property, cs_int32 value);
void CPE_WriteSetEntityProperty(Client client, Client other, cs_int8 type, cs_int32 value);
void CPE_WriteTwoWayPing(Client client, cs_uint8 direction, cs_int16 num);
void CPE_WriteSetModel(Client client, Client other);
void CPE_WriteBlockPerm(Client client, BlockID id, cs_bool allowPlace, cs_bool allowDestroy);
#endif // PROTOCOL_H
