#ifndef WORLD_H
#define WORLD_H
#include "core.h"
#include "platform.h"
#include "vector.h"
#include "list.h"
#include "compr.h"

/*
** Если какой-то из дефайнов ниже
** вырос, удостовериться, что
** int-типа в структуре _WorldInfo
** хватает для представления всех
** этих значений в степени двойки.
*/
#define WORLD_PROPS_COUNT 10
#define WORLD_COLORS_COUNT 5

typedef enum _EWorldColor {
	WORLD_COLOR_SKY,
	WORLD_COLOR_CLOUD,
	WORLD_COLOR_FOG,
	WORLD_COLOR_AMBIENT,
	WORLD_COLOR_DIFFUSE
} EWorldColors;

typedef enum _EWorldProp {
	WORLD_PROP_SIDEBLOCK,
	WORLD_PROP_EDGEBLOCK,
	WORLD_PROP_EDGELEVEL,
	WORLD_PROP_CLOUDSLEVEL,
	WORLD_PROP_FOGDIST,
	WORLD_PROP_SPDCLOUDS,
	WORLD_PROP_SPDWEATHER,
	WORLD_PROP_FADEWEATHER,
	WORLD_PROP_EXPFOG,
	WORLD_PROP_SIDEOFFSET
} EWorldProp;

typedef enum _EWorldWeather {
	WORLD_WEATHER_SUN,
	WORLD_WEATHER_RAIN,
	WORLD_WEATHER_SNOW
} EWorldWeather;

#define MV_NONE    0x00
#define MV_COLORS  BIT(0)
#define MV_PROPS   BIT(1)
#define MV_TEXPACK BIT(2)
#define MV_WEATHER BIT(3)

typedef enum _EWorldError {
	WORLD_ERROR_SUCCESS = 0,
	WORLD_ERROR_IOFAIL,
	WORLD_ERROR_COMPR,
	WORLD_ERROR_INFOREAD,
	WORLD_ERROR_DATAREAD
} EWorldError;

typedef enum _EWorldExtra {
	WORLD_EXTRA_NOINFO = 0,
	WORLD_EXTRA_UNKNOWN_DATA_TYPE,
	WORLD_EXTRA_IO_OPEN,
	WORLD_EXTRA_IO_WRITE,
	WORLD_EXTRA_COMPR_INIT,
	WORLD_EXTRA_COMPR_PROC
} EWorldExtra;

typedef struct _WorldInfo {
	SVec dimensions;
	Color3 colors[WORLD_COLORS_COUNT];
	cs_int32 props[WORLD_PROPS_COUNT];
	cs_char texturepack[65];
	Vec spawnVec;
	Ang spawnAng;
	cs_int8 weatherType;
	cs_byte modval, modclr;
	cs_uint16 modprop;
} WorldInfo;

typedef struct _World {
	cs_str name;
	WorldInfo info;
	cs_bool modified;
	Waitable *waitable;
	cs_bool loaded;
	cs_bool saveUnload;
	Compr compr;
	KListField *headNode;
	struct _WorldError {
		EWorldError code;
		EWorldExtra extra;
	} error;
	struct _WorldData {
		cs_uint32 size;
		void *ptr;
		BlockID *blocks;
	} wdata;
} World;

API cs_bool World_HasError(World *world);
API EWorldError World_PopError(World *world, EWorldExtra *extra);

API World *World_Create(cs_str name);
API void World_AllocBlockArray(World *world);
API void World_FreeBlockArray(World *world);
API void World_Free(World *world);
API void World_Add(World *world);
API cs_bool World_IsReadyToPlay(World *world);
API void World_UpdateClients(World *world);

API cs_bool World_Load(World *world);
API void World_Unload(World *world);
API cs_bool World_Save(World *world, cs_bool unload);

API void World_SetDimensions(World *world, const SVec *dims);
API cs_bool World_SetBlock(World *world, SVec *pos, BlockID id);
API cs_bool World_SetBlockO(World *world, cs_uint32 offset, BlockID id);
API cs_bool World_SetEnvColor(World *world, EWorldColors type, Color3* color);
API cs_bool World_SetProperty(World *world, EWorldProp property, cs_int32 value);
API cs_bool World_SetTexturePack(World *world, cs_str url);
API cs_bool World_SetWeather(World *world, EWorldWeather type);

API cs_str World_GetName(World *world);
API void *World_GetData(World *world, cs_uint32 *size);
API BlockID *World_GetBlockArray(World *world, cs_uint32 *size);
API cs_uint32 World_GetBlockArraySize(World *world);
API cs_uint32 World_GetOffset(World *world, SVec *pos);
API void World_GetDimensions(World *world, SVec *dims);
API BlockID World_GetBlock(World *world, SVec *pos);
API BlockID World_GetBlockO(World *world, cs_uint32 offset);
API cs_int32 World_GetProperty(World *world, EWorldProp property);
API Color3* World_GetEnvColor(World *world, EWorldColors type);
API EWorldWeather World_GetWeather(World *world);
API cs_str World_GetTexturePack(World *world);

API World *World_GetByName(cs_str name);

VAR AListField *World_Head;
#endif // WORLD_H
