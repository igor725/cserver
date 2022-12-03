#ifndef WORLD_H
#define WORLD_H
#include "core.h"
#include "vector.h"
#include "types/list.h"
#include "types/world.h"
#include "types/cpe.h"

API cs_bool World_HasError(World *world);
API EWorldError World_PopError(World *world, EWorldExtra *extra);

API World *World_Create(cs_str name);
API void World_AllocBlockArray(World *world);
API cs_bool World_CleanBlockArray(World *world);
API void World_FreeBlockArray(World *world);
API void World_Free(World *world);
API void World_Add(World *world);
API cs_bool World_Remove(World *world);
API cs_bool World_IsReadyToPlay(World *world);
API cs_bool World_IsInMemory(World *world);
API cs_bool World_IsModified(World *world);
API cs_bool World_FinishEnvUpdate(World *world);
API cs_byte World_CountPlayers(World *world);

API cs_bool World_Load(World *world);
API void World_Unload(World *world);
API cs_bool World_Save(World *world);

API cs_bool World_Lock(World *world, cs_ulong timeout);
API void World_Unlock(World *world);
API void World_StartTask(World *world);
API void World_EndTask(World *world);
API void World_WaitAllTasks(World *world);

API void World_SetInMemory(World *world, cs_bool state);
API void World_SetIgnoreModifications(World *world, cs_bool state);
API void World_SetSpawn(World *world, Vec *svec, Ang *sang);
API cs_bool World_SetDimensions(World *world, const SVec *dims);
API cs_bool World_SetBlock(World *world, SVec *pos, BlockID id);
API cs_bool World_SetBlockO(World *world, cs_uint32 offset, BlockID id);
API cs_bool World_SetEnvColor(World *world, EColor type, Color3* color);
API cs_bool World_SetEnvProp(World *world, EProp prop, cs_int32 value);
API cs_bool World_SetTexturePack(World *world, cs_str url);
API cs_bool World_SetWeather(World *world, EWeather type);
API void World_SetSeed(World *world, cs_uint32 seed);

API cs_str World_GetName(World *world);
API void World_GetSpawn(World *world, Vec *svec, Ang *sang);
API void *World_GetData(World *world, cs_uint32 *size);
API BlockID *World_GetBlockArray(World *world, cs_uint32 *size);
API cs_uint32 World_GetBlockArraySize(World *world);
API cs_uint32 World_GetOffset(World *world, SVec *pos);
API void World_GetDimensions(World *world, SVec *dims);
API BlockID World_GetBlock(World *world, SVec *pos);
API BlockID World_GetBlockO(World *world, cs_uint32 offset);
API cs_int32 World_GetEnvProp(World *world, EProp prop);
API cs_bool World_GetEnvColor(World *world, EColor type, Color3 *dst);
API EWeather World_GetWeather(World *world);
API cs_str World_GetTexturePack(World *world);
API cs_uint32 World_GetSeed(World *world);

API World *World_GetByName(cs_str name);

VAR World *World_Main;
VAR AListField *World_Head;
#endif // WORLD_H
