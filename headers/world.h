#ifndef WORLD_H
#define WORLD_H
#include "platform.h"
#include "vector.h"
/*
** Если какой-то из дефайнов ниже
** вырос, удостовериться, что
** int-типа в структуре worldInfo
** хватает для представления всех
** этих значений в степени двойки.
*/
#define WORLD_PROPS_COUNT 10
#define WORLD_COLORS_COUNT 5

enum ColorTypes {
	COLOR_SKY,
	COLOR_CLOUD,
	COLOR_FOG,
	COLOR_AMBIENT,
	COLOR_DIFFUSE
};

enum MapEnvProps {
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

enum WeatherTypes {
	WEATHER_SUN,
	WEATHER_RAIN,
	WEATHER_SNOW
};

enum WorldDataType {
	DT_DIM,
	DT_SV,
	DT_SA,
	DT_WT,
	DT_PROPS,
	DT_COLORS,

	DT_END = 0xFF
};

enum ModifiedValues {
	MV_COLORS = 1,
	MV_PROPS = 2,
	MV_TEXPACK = 4,
	MV_WEATHER = 8
};

enum WorldProcesses {
	WP_NOPROC,
	WP_SAVING,
	WP_LOADING,
};

typedef struct worldInfo {
	SVec dimensions;
	Color3 colors[WORLD_COLORS_COUNT];
	cs_int32 props[WORLD_PROPS_COUNT];
	char texturepack[65];
	Vec spawnVec;
	Ang spawnAng;
	Weather wt;
	cs_uint8 modval;
	cs_uint16 modprop;
	cs_uint8 modclr;
} *WorldInfo;

typedef struct world {
	WorldID id;
	const char* name;
	cs_uint32 size;
	WorldInfo info;
	cs_bool modified;
	Waitable wait;
	cs_bool loaded;
	cs_bool saveUnload;
	cs_int32 process;
	BlockID* data;
} *World;

API void Worlds_SaveAll(cs_bool join, cs_bool unload);

API World World_Create(const char* name);
API void World_AllocBlockArray(World world);
API void World_Free(World world);
API cs_bool World_Add(World world);
API void World_UpdateClients(World world);

API cs_bool World_Load(World world);
API void World_Unload(World world);
API cs_bool World_Save(World world, cs_bool unload);

API void World_SetDimensions(World world, const SVec* dims);
API cs_bool World_SetBlock(World world, SVec* pos, BlockID id);
API cs_bool World_SetEnvColor(World world, cs_uint8 type, Color3* color);
API cs_bool World_SetEnvProperty(World world, cs_uint8 property, cs_int32 value);
API cs_bool World_SetTexturePack(World world, const char* url);
API cs_bool World_SetWeather(World world, Weather type);

API cs_uint32 World_GetOffset(World world, SVec* pos);
API BlockID World_GetBlock(World world, SVec* pos);
API cs_int32 World_GetProperty(World world, cs_uint8 property);
API Color3* World_GetEnvColor(World world, cs_uint8 type);
API Weather World_GetWeather(World world);
API World World_GetByName(const char* name);
API World World_GetByID(WorldID id);

VAR World Worlds_List[MAX_WORLDS];
#endif
