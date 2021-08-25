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

enum {
	COLOR_SKY,
	COLOR_CLOUD,
	COLOR_FOG,
	COLOR_AMBIENT,
	COLOR_DIFFUSE
};

enum {
	PROP_SIDEBLOCK,
	PROP_EDGEBLOCK,
	PROP_EDGELEVEL,
	PROP_CLOUDSLEVEL,
	PROP_FOGDIST,
	PROP_SPDCLOUDS,
	PROP_SPDWEATHER,
	PROP_FADEWEATHER,
	PROP_EXPFOG,
	PROP_SIDEOFFSET
};

enum {
	WEATHER_SUN,
	WEATHER_RAIN,
	WEATHER_SNOW
};

enum {
	DT_DIM,
	DT_SV,
	DT_SA,
	DT_WT,
	DT_PROPS,
	DT_COLORS,

	DT_END = 0xFF
};

enum {
	MV_NONE = 0,
	MV_COLORS = BIT(0),
	MV_PROPS = BIT(1),
	MV_TEXPACK = BIT(2),
	MV_WEATHER = BIT(3)
};

enum {
	WP_NOPROC,
	WP_SAVING,
	WP_LOADING,
};

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
	struct _WorldData {
		cs_uint32 size;
		void *ptr;
		BlockID *blocks;
	} wdata;
} World;

API World *World_Create(cs_str name);
API void World_AllocBlockArray(World *world);
API void World_FreeBlockArray(World *world);
API void World_Free(World *world);
API cs_bool World_Add(World *world);
API cs_bool World_IsReadyToPlay(World *world);
API void World_UpdateClients(World *world);

API cs_bool World_Load(World *world);
API void World_Unload(World *world);
API cs_bool World_Save(World *world, cs_bool unload);

API void World_SetDimensions(World *world, const SVec *dims);
API cs_bool World_SetBlock(World *world, SVec *pos, BlockID id);
API cs_bool World_SetBlockO(World *world, cs_uint32 offset, BlockID id);
API cs_bool World_SetEnvColor(World *world, cs_byte type, Color3* color);
API cs_bool World_SetProperty(World *world, cs_byte property, cs_int32 value);
API cs_bool World_SetTexturePack(World *world, cs_str url);
API cs_bool World_SetWeather(World *world, cs_int8 type);

API cs_str World_GetName(World *world);
API void *World_GetData(World *world, cs_uint32 *size);
API BlockID *World_GetBlockArray(World *world, cs_uint32 *size);
API cs_uint32 World_GetBlockArraySize(World *world);
API cs_uint32 World_GetOffset(World *world, SVec *pos);
API BlockID World_GetBlock(World *world, SVec *pos);
API cs_int32 World_GetProperty(World *world, cs_byte property);
API Color3* World_GetEnvColor(World *world, cs_byte type);
API cs_int8 World_GetWeather(World *world);

API World *World_GetByName(cs_str name);

VAR AListField *World_Head;
#endif // WORLD_H
