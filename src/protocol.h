#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "core.h"
#include "client.h"

#define ValidateClientState(client, st, ret) \
if(!client->playerData || client->playerData->state != st) \
	return ret;

#define ValidateCpeClient(client, ret) \
if(!client->cpeData) return ret;

#define PacketWriter_Start(client) \
if(client->closed) return; \
cs_char *data = client->wrbuf; \
Mutex_Lock(client->mutex);

#define PacketWriter_End(client, size) \
Client_Send(client, size); \
Mutex_Unlock(client->mutex);

#define PacketWriter_Stop(client) \
Mutex_Unlock(client->mutex); \
return;

#define CLIENT_SELF (cs_int8)-1

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
#define EXT_SETSPAWN 0x9149FD59ul
#define EXT_VELCTRL 0xF8DF4FF7ul
#define EXT_PARTICLE 0x0D732743ul

typedef cs_bool(*packetHandler)(Client *, cs_char *);

typedef struct _Packet {
	cs_byte id;
	cs_bool haveCPEImp;
	cs_uint16 size, extSize;
	cs_uint32 exthash;
	cs_int32 extVersion;
	packetHandler handler;
	packetHandler cpeHandler;
} Packet;

Packet *Packet_Get(cs_byte id);
API void Packet_Register(cs_byte id, cs_uint16 size, packetHandler handler);
API void Packet_RegisterCPE(cs_byte id, cs_uint32 hash, cs_int32 ver, cs_uint16 size, packetHandler handler);
API void Packet_RegisterExtension(cs_str name, cs_int32 version);

API cs_byte Proto_ReadString(cs_char **data, cs_str *dstptr);
API cs_byte Proto_ReadStringNoAlloc(cs_char **data, cs_char *dst);
API void Proto_ReadSVec(cs_char **dataptr, SVec *vec);
API void Proto_ReadAng(cs_char **dataptr, Ang *ang);
API void Proto_ReadFlSVec(cs_char **dataptr, Vec *vec);
API void Proto_ReadFlVec(cs_char **dataptr, Vec *vec);
API cs_bool Proto_ReadClientPos(Client *client, cs_char *data);

API void Proto_WriteString(cs_char **dataptr, cs_str string);
API void Proto_WriteFlVec(cs_char **dataptr, const Vec *vec);
API void Proto_WriteFlSVec(cs_char **dataptr, const Vec *vec);
API void Proto_WriteSVec(cs_char **dataptr, const SVec *vec);
API void Proto_WriteAng(cs_char **dataptr, const Ang *ang);
API void Proto_WriteColor3(cs_char **dataptr, const Color3* color);
API void Proto_WriteColor4(cs_char **dataptr, const Color4* color);
API void Proto_WriteByteColor3(cs_char **dataptr, const Color3* color);
API void Proto_WriteByteColor4(cs_char **dataptr, const Color4* color);
API cs_uint32 Proto_WriteClientPos(cs_char *data, Client *client, cs_bool extended);

/*
** Врайтеры и хендлеры
** ванильного протокола
*/

NOINL void Vanilla_WriteHandshake(Client *client, cs_str name, cs_str motd);
NOINL void Vanilla_WriteLvlInit(Client *client, cs_uint32 size);
NOINL void Vanilla_WriteLvlFin(Client *client, SVec *dims);
NOINL void Vanilla_WriteSetBlock(Client *client, SVec *pos, BlockID block);
NOINL void Vanilla_WriteSpawn(Client *client, Client *other);
NOINL void Vanilla_WriteTeleport(Client *client, Vec *pos, Ang *ang);
NOINL void Vanilla_WritePosAndOrient(Client *client, Client *other);
NOINL void Vanilla_WriteDespawn(Client *client, Client *other);
NOINL void Vanilla_WriteChat(Client *client, cs_byte type, cs_str mesg);
NOINL void Vanilla_WriteKick(Client *client, cs_str reason);

/*
** Врайтеры и хендлеры
** CPE протокола и прочие,
** связанные с CPE вещи
*/
API cs_bool CPE_CheckModel(cs_int16 model);
API void CPE_RegisterExtension(cs_str name, cs_int32 version);
API cs_int16 CPE_GetModelNum(cs_str model);
API cs_str CPE_GetModelStr(cs_int16 num);

NOINL void CPE_WriteInfo(Client *client);
NOINL void CPE_WriteExtEntry(Client *client, CPEExt *ext);
NOINL void CPE_WriteClickDistance(Client *client, cs_int16 dist);
NOINL void CPE_WriteInventoryOrder(Client *client, cs_byte order, BlockID block);
NOINL void CPE_WriteHoldThis(Client *client, BlockID block, cs_bool preventChange);
NOINL void CPE_WriteSetHotKey(Client *client, cs_str action, cs_int32 keycode, cs_int8 keymod);
NOINL void CPE_WriteAddName(Client *client, Client *other);
NOINL void CPE_WriteAddEntity2(Client *client, Client *other);
NOINL void CPE_WriteRemoveName(Client *client, Client *other);
NOINL void CPE_WriteEnvColor(Client *client, cs_byte type, Color3* col);
NOINL void CPE_WriteMakeSelection(Client *client, cs_byte id, SVec *start, SVec *end, Color4* color);
NOINL void CPE_WriteRemoveSelection(Client *client, cs_byte id);
NOINL void CPE_WriteHackControl(Client *client, CPEHacks *hacks);
NOINL void CPE_WriteDefineBlock(Client *client, BlockDef *block);
NOINL void CPE_WriteUndefineBlock(Client *client, BlockID id);
NOINL void CPE_WriteDefineExBlock(Client *client, BlockDef *block);
NOINL void CPE_WriteBulkBlockUpdate(Client *client, BulkBlockUpdate *bbu);
NOINL void CPE_WriteSetTextColor(Client *client, Color4* color, cs_char code);
NOINL void CPE_WriteSetHotBar(Client *client, cs_byte order, BlockID block);
NOINL void CPE_WriteSetSpawnPoint(Client *client, Vec *pos, Ang *ang);
NOINL void CPE_WriteVelocityControl(Client *client, Vec *velocity, cs_bool mode);
NOINL void CPE_WriteDefineEffect(Client *client, CustomParticle *e);
NOINL void CPE_WriteSpawnEffect(Client *client, cs_byte id, Vec *pos, Vec *origin);
NOINL void CPE_WriteWeatherType(Client *client, cs_int8 type);
NOINL void CPE_WriteTexturePack(Client *client, cs_str url);
NOINL void CPE_WriteMapProperty(Client *client, cs_byte property, cs_int32 value);
NOINL void CPE_WriteSetEntityProperty(Client *client, Client *other, cs_int8 type, cs_int32 value);
NOINL void CPE_WriteTwoWayPing(Client *client, cs_byte direction, cs_int16 num);
NOINL void CPE_WriteSetModel(Client *client, Client *other);
NOINL void CPE_WriteBlockPerm(Client *client, BlockID id, cs_bool allowPlace, cs_bool allowDestroy);

void Packet_RegisterDefault(void);
#endif // PROTOCOL_H
