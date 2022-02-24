#ifndef CLIENT_H
#define CLIENT_H
#include "core.h"
#include "vector.h"
#include "types/list.h"
#include "types/world.h"
#include "types/websock.h"
#include "types/compr.h"
#include "types/growingbuffer.h"
#include "types/block.h"
#include "types/cpe.h"
#include "types/client.h"
#include <stdarg.h>

void Client_Tick(Client *client);
void Client_Free(Client *client);

NOINL cs_bool Client_RawSend(Client *client, cs_char *buf, cs_int32 len);
API void Client_SetNoFlush(Client *client, cs_bool state);
NOINL cs_bool Client_SendAnytimeData(Client *client, cs_int32 size);
NOINL cs_bool Client_FlushBuffer(Client *client);

API void Client_BulkBlockUpdate(Client *client, BulkBlockUpdate *bbu);
NOINL cs_bool Client_DefineBlock(Client *client, BlockDef *block);
NOINL cs_bool Client_UndefineBlock(Client *client, BlockID id);

API CGroup *Group_Add(cs_int16 gid, cs_str gname, cs_byte grank);
API CGroup *Group_GetByID(cs_int16 gid);
API cs_bool Group_Remove(cs_int16 gid);

API cs_byte Clients_GetCount(EPlayerState state);
API void Clients_KickAll(cs_str reason);

API cs_bool Client_ChangeWorld(Client *client, World *world);
API void Client_Chat(Client *client, EMesgType type, cs_str message);
API void Client_Kick(Client *client, cs_str reason);
API void Client_KickFormat(Client *client, cs_str fmtreason, ...);
API void Client_UpdateWorldInfo(Client *client, World *world, cs_bool updateAll);
API cs_bool Client_Update(Client *client);
API cs_bool Client_SendHacks(Client *client, CPEHacks *hacks);
API cs_bool Client_MakeSelection(Client *client, cs_byte id, SVec *start, SVec *end, Color4* color);
API cs_bool Client_RemoveSelection(Client *client, cs_byte id);
API cs_bool Client_TeleportTo(Client *client, Vec *pos, Ang *ang);
API cs_bool Client_TeleportToSpawn(Client *client);
API cs_bool Client_CheckState(Client *client, EPlayerState state);

API cs_bool Client_IsLocal(Client *client);
API cs_bool Client_IsInSameWorld(Client *client, Client *other);
API cs_bool Client_IsInWorld(Client *client, World *world);
API cs_bool Client_IsOP(Client *client);
API cs_bool Client_IsFirstSpawn(Client *client);

API cs_bool Client_SetDisplayName(Client *client, cs_str name);
API cs_bool Client_SetWeather(Client *client, cs_int8 type);
API cs_bool Client_SetInvOrder(Client *client, cs_byte order, BlockID block);
API cs_bool Client_SetServerIdent(Client *client, cs_str name, cs_str motd);
API cs_bool Client_SetEnvProperty(Client *client, cs_byte property, cs_int32 value);
API cs_bool Client_SetEnvColor(Client *client, cs_byte type, Color3* color);
API cs_bool Client_SetTexturePack(Client *client, cs_str url);
API cs_bool Client_AddTextColor(Client *client, Color4* color, cs_char code);
API void Client_SetBlock(Client *client, SVec *pos, BlockID id);
API cs_bool Client_SetModel(Client *client, cs_int16 model);
API cs_bool Client_SetModelStr(Client *client, cs_str model);
API cs_bool Client_SetBlockPerm(Client *client, BlockID block, cs_bool allowPlace, cs_bool allowDestroy);
API cs_bool Client_SetHeldBlock(Client *client, BlockID block, cs_bool preventChange);
API cs_bool Client_SetClickDistance(Client *client, cs_uint16 dist);
API cs_bool Client_SetHotkey(Client *client, cs_str action, cs_int32 keycode, cs_int8 keymod);
API cs_bool Client_SetHotbar(Client *client, cs_byte pos, BlockID block);
API cs_bool Client_SetSkin(Client *client, cs_str skin);
API cs_bool Client_SetSpawn(Client *client, Vec *pos, Ang *ang);
API cs_bool Client_SetOP(Client *client, cs_bool state);
API cs_bool Client_SetVelocity(Client *client, Vec *velocity, cs_bool mode);
API cs_bool Client_SetRotation(Client *client, cs_byte axis, cs_int32 value);
API cs_bool Client_SetGroup(Client *client, cs_int16 gid);
API cs_bool Client_RegisterParticle(Client *client, CustomParticle *e);
API cs_bool Client_SpawnParticle(Client *client, cs_byte id, Vec *pos, Vec *origin);

API cs_str Client_GetName(Client *client);
API cs_str Client_GetDisplayName(Client *client);
API cs_str Client_GetAppName(Client *client);
API cs_str Client_GetKey(Client *client);
API cs_str Client_GetSkin(Client *client);
API cs_str Client_GetDisconnectReason(Client *client);
API ClientID Client_GetID(Client *client);
API Client *Client_GetByID(ClientID id);
API Client *Client_GetByName(cs_str name);
API World *Client_GetWorld(Client *client);
API BlockID Client_GetStandBlock(Client *client);
API cs_int8 Client_GetFluidLevel(Client *client, BlockID *fluid);
API cs_int16 Client_GetModel(Client *client);
API BlockID Client_GetHeldBlock(Client *client);
API cs_uint16 Client_GetClickDistance(Client *client);
API cs_float Client_GetClickDistanceInBlocks(Client *client);
API cs_bool Client_GetPosition(Client *client, Vec *pos, Ang *ang);
API cs_int32 Client_GetExtVer(Client *client, cs_uint32 exthash);
API cs_uint32 Client_GetAddr(Client *client);
API cs_int32 Client_GetPing(Client *client);
API cs_float Client_GetAvgPing(Client *client);
API CGroup *Client_GetGroup(Client *client);
API cs_int16 Client_GetGroupID(Client *client);

API cs_bool Client_Spawn(Client *client);
API cs_bool Client_Despawn(Client *client);

VAR Client *Broadcast;
VAR Client *Clients_List[MAX_CLIENTS];
#endif // CLIENT_H
