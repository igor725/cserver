#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "core.h"
#include "types/cpe.h"
#include "types/client.h"
#include "types/protocol.h"
#include "types/block.h"
#include "types/keys.h"

typedef cs_bool(*packetHandler)(Client *, cs_char *);

API cs_bool Packet_Register(EPacketID id, cs_uint16 size, packetHandler handler);
API cs_bool Packet_SetCPEHandler(EPacketID id, cs_uint32 hash, cs_int32 ver, cs_uint16 size, packetHandler handler);
Packet *Packet_Get(EPacketID id);
void Packet_UnregisterAll(void);

API cs_byte Proto_ReadString(cs_char **data, cs_str *dstptr);
API cs_byte Proto_ReadStringNoAlloc(cs_char **data, cs_char *dst);
API void Proto_ReadSVec(cs_char **dataptr, SVec *vec);
API void Proto_ReadAng(cs_char **dataptr, Ang *ang);
API void Proto_ReadFlSVec(cs_char **dataptr, Vec *vec);
API void Proto_ReadFlVec(cs_char **dataptr, Vec *vec);

API void Proto_WriteString(cs_char **dataptr, cs_str string);
API void Proto_WriteFlVec(cs_char **dataptr, const Vec *vec);
API void Proto_WriteFlSVec(cs_char **dataptr, const Vec *vec);
API void Proto_WriteSVec(cs_char **dataptr, const SVec *vec);
API void Proto_WriteAng(cs_char **dataptr, const Ang *ang);
API void Proto_WriteColor3(cs_char **dataptr, const Color3* color);
API void Proto_WriteColor4(cs_char **dataptr, const Color4* color);
API void Proto_WriteByteColor3(cs_char **dataptr, const Color3* color);
API void Proto_WriteByteColor4(cs_char **dataptr, const Color4* color);
API void Proto_WriteFloat(cs_char **dataptr, cs_float num);

/*
 * Врайтеры и хендлеры
 * ванильного протокола
*/

NOINL void Vanilla_WriteServerIdent(Client *client, cs_str name, cs_str motd);
NOINL void Vanilla_WriteLvlInit(Client *client);
NOINL void Vanilla_WriteLvlFin(Client *client, SVec *dims);
NOINL void Vanilla_WriteSetBlock(Client *client, SVec *pos, BlockID block);
NOINL void Vanilla_WriteSpawn(Client *client, Client *other);
NOINL void Vanilla_WriteTeleport(Client *client, Vec *pos, Ang *ang);
NOINL void Vanilla_WritePosAndOrient(Client *client, Client *other);
NOINL void Vanilla_WriteDespawn(Client *client, Client *other);
NOINL void Vanilla_WriteChat(Client *client, EMesgType type, cs_str mesg);
NOINL void Vanilla_WriteKick(Client *client, cs_str reason);
NOINL void Vanilla_WriteUserType(Client *client, cs_byte type);

/*
 * Врайтеры и хендлеры
 * CPE протокола и прочие,
 * связанные с CPE вещи
*/

NOINL void CPE_WriteInfo(Client *client);
NOINL void CPE_WriteExtEntry(Client *client, CPESvExt *ext);
NOINL void CPE_WriteClickDistance(Client *client, cs_uint16 dist);
NOINL void CPE_CustomBlockSupportLevel(Client *client, cs_byte level);
NOINL void CPE_WriteInventoryOrder(Client *client, cs_byte order, BlockID block);
NOINL void CPE_WriteHoldThis(Client *client, BlockID block, cs_bool preventChange);
NOINL void CPE_WriteSetHotKey(Client *client, cs_str action, ELWJGLKey keycode, ELWJGLMod keymod);
NOINL void CPE_WriteAddName(Client *client, Client *other);
NOINL void CPE_WriteAddEntity_v1(Client *client, Client *other);
NOINL void CPE_WriteAddEntity_v2(Client *client, Client *other);
NOINL void CPE_WriteRemoveName(Client *client, Client *other);
NOINL void CPE_WriteEnvColor(Client *client, cs_byte type, Color3* col);
NOINL void CPE_WriteMakeSelection(Client *client, CPECuboid *cub);
NOINL void CPE_WriteRemoveSelection(Client *client, cs_byte id);
NOINL void CPE_WriteHackControl(Client *client, CPEHacks *hacks);
NOINL void CPE_WriteDefineBlock(Client *client, BlockID id, BlockDef *block);
NOINL void CPE_WriteUndefineBlock(Client *client, BlockID id);
NOINL void CPE_WriteDefineExBlock(Client *client, BlockID id, BlockDef *block);
NOINL void CPE_WriteBulkBlockUpdate(Client *client, BulkBlockUpdate *bbu);
NOINL void CPE_WriteFastMapInit(Client *client, cs_uint32 size);
NOINL void CPE_WriteAddTextColor(Client *client, Color4* color, cs_char code);
NOINL void CPE_WriteSetHotBar(Client *client, cs_byte order, BlockID block);
NOINL void CPE_WriteSetSpawnPoint(Client *client, Vec *pos, Ang *ang);
NOINL void CPE_WriteVelocityControl(Client *client, Vec *velocity, cs_byte mode);
NOINL void CPE_WriteDefineEffect(Client *client, cs_byte id, CPEParticle *e);
NOINL void CPE_WriteSpawnEffect(Client *client, cs_byte id, Vec *pos, Vec *origin);
NOINL void CPE_WriteWeatherType(Client *client, cs_int8 type);
NOINL void CPE_WriteTexturePack(Client *client, cs_str url);
NOINL void CPE_WriteMapProperty(Client *client, cs_byte property, cs_int32 value);
NOINL void CPE_WriteSetEntityProperty(Client *client, Client *other, EEntProp type, cs_int32 value);
NOINL void CPE_WriteTwoWayPing(Client *client, cs_byte direction, cs_int16 num);
NOINL void CPE_WriteSetModel(Client *client, Client *other);
NOINL void CPE_WriteSetMapAppearanceV1(Client *client, cs_str tex, cs_byte side, cs_byte edge, cs_int16 sidelvl);
NOINL void CPE_WriteSetMapAppearanceV2(Client *client, cs_str tex, cs_byte side, cs_byte edge, cs_int16 sidelvl, cs_int16 cllvl, cs_int16 maxview);
NOINL void CPE_WriteBlockPerm(Client *client, BlockID id, cs_bool allowPlace, cs_bool allowDestroy);
NOINL void CPE_WriteDefineModel(Client *client, cs_byte id, CPEModel *mdoel);
NOINL void CPE_WriteDefineModelPart(Client *client, cs_int32 ver, cs_byte id, CPEModelPart *part);
NOINL void CPE_WriteUndefineModel(Client *client, cs_byte id);
NOINL void CPE_WritePluginMessage(Client *client, cs_byte channel, cs_str message);

API void CPE_RegisterServerExtension(cs_str name, cs_int32 version);
void Packet_RegisterDefault(void);
#endif // PROTOCOL_H
