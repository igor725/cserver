#ifndef WORLD_H
#define WORLD_H
#define WORLD_PROPS_COUNT 9

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

	DT_END = 0xFF
};

typedef struct worldDims {
	uint16_t width, height, length;
} *WORLDDIMS;

typedef struct worldInfo {
	WORLDDIMS   dim;
	int         props[WORLD_PROPS_COUNT];
	char        texturepack[64];
	VECTOR      spawnVec;
	ANGLE       spawnAng;
	Weather     wt;
} *WORLDINFO;

typedef struct world {
	int         id;
	const char* name;
	uint32_t    size;
	BlockID*    data;
	WORLDINFO   info;
	THREAD      thread;
	bool        saveUnload;
	bool        saveDone;
} *WORLD;

void World_Tick(WORLD world);

API WORLD World_Create(const char* name);
API void World_AllocBlockArray(WORLD world);
API void World_Free(WORLD world);
API bool World_Add(WORLD world);

API bool World_Load(WORLD world);
API bool World_Save(WORLD world);

API void World_SetDimensions(WORLD world, uint16_t width, uint16_t height, uint16_t length);
API bool World_SetBlock(WORLD world, uint16_t x, uint16_t y, uint16_t z, BlockID id);
API bool World_SetProperty(WORLD world, uint8_t property, int value);
API bool World_SetTexturePack(WORLD world, const char* url);
API bool World_SetWeather(WORLD world, Weather type);

API uint32_t World_GetOffset(WORLD world, uint16_t x, uint16_t y, uint16_t z);
API BlockID World_GetBlock(WORLD world, uint16_t x, uint16_t y, uint16_t z);
API int World_GetProperty(WORLD world, uint8_t property);
API Weather World_GetWeather(WORLD world);
API WORLD World_GetByName(const char* name);
API WORLD World_GetByID(int id);

VAR WORLD Worlds_List[MAX_WORLDS];
#endif
