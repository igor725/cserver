#ifndef WORLD_H
#define WORLD_H
/*
** Если какой-то из дефайнов ниже
** вырос, удостовериться, что
** int-типа в структуре worldInfo
** хватает для представления всех
** этих значений в степени двойки.
*/
#define WORLD_PROPS_COUNT 10
#define WORLD_COLORS_COUNT 5 * 3 // 5 типов и 3 значения r,g,b

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

typedef struct worldInfo {
	uint16_t width, height, length;
	int16_t colors[WORLD_COLORS_COUNT];
	int32_t props[WORLD_PROPS_COUNT];
	char texturepack[65];
	struct vector spawnVec;
	struct angle spawnAng;
	Weather wt;
	uint8_t modval;
	uint16_t modprop;
	uint8_t modclr;
} *WORLDINFO;

typedef struct world {
	int32_t id;
	const char* name;
	uint32_t size;
	WORLDINFO info;
	bool modified;
	THREAD thread;
	bool saveUnload;
	bool saveDone;
	BlockID* data;
} *WORLD;

void World_Tick(WORLD world);

API void Worlds_SaveAll(bool join);

API WORLD World_Create(const char* name);
API void World_AllocBlockArray(WORLD world);
API void World_Free(WORLD world);
API bool World_Add(WORLD world);
API void World_UpdateClients(WORLD world);

API bool World_Load(WORLD world);
API bool World_Save(WORLD world);

API void World_SetDimensions(WORLD world, uint16_t width, uint16_t height, uint16_t length);
API bool World_SetBlock(WORLD world, uint16_t x, uint16_t y, uint16_t z, BlockID id);
API bool World_SetProperty(WORLD world, uint8_t property, int32_t value);
API bool World_SetTexturePack(WORLD world, const char* url);
API bool World_SetWeather(WORLD world, Weather type);

API uint32_t World_GetOffset(WORLD world, uint16_t x, uint16_t y, uint16_t z);
API BlockID World_GetBlock(WORLD world, uint16_t x, uint16_t y, uint16_t z);
API int32_t World_GetProperty(WORLD world, uint8_t property);
API int16_t* World_GetColor(WORLD world, uint8_t type);
API Weather World_GetWeather(WORLD world);
API WORLD World_GetByName(const char* name);
API WORLD World_GetByID(int32_t id);

VAR WORLD Worlds_List[MAX_WORLDS];
#endif
