#ifndef WORLD_H
#define WORLD_H
enum WorldDataType {
	DT_DIM,
	DT_SV,
	DT_SA,
	DT_WT,

	DT_END = 0xFF
};

typedef struct worldDims {
	uint16_t width, height, length;
} WORLDDIMS;

typedef struct worldInfo {
	WORLDDIMS*  dim;
	VECTOR*     spawnVec;
	ANGLE*      spawnAng;
	Weather     wt;
} WORLDINFO;

typedef struct world {
	const char* name;
	uint32_t    size;
	BlockID*    data;
	WORLDINFO*  info;
} WORLD;

API WORLD* World_Create(const char* name);
API uint32_t World_GetOffset(WORLD* world, uint16_t x, uint16_t y, uint16_t z);
API void World_SetDimensions(WORLD* world, uint16_t width, uint16_t height, uint16_t length);
API int World_SetBlock(WORLD* world, uint16_t x, uint16_t y, uint16_t z, BlockID id);
API BlockID World_GetBlock(WORLD* world, uint16_t x, uint16_t y, uint16_t z);
API void World_GenerateFlat(WORLD* world);
API WORLD* World_FindByName(const char* name);
API void World_Destroy(WORLD* world);
API bool World_Load(WORLD* world);
API bool World_Save(WORLD* world);
API void World_AllocBlockArray(WORLD* world);
API void World_SetWeather(WORLD* world, Weather type);
API Weather World_GetWeather(WORLD* world);

WORLD* Worlds_List[MAX_WORLDS];
#endif
