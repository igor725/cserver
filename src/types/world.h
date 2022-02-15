#ifndef WORLDTYPES_H
#define WORLDTYPES_H
#include "core.h"
#include "vector.h"
#include "types/list.h"
#include "types/platform.h"
#include "types/compr.h"
#include "types/cpe.h"

#define MV_NONE    0x00
#define MV_COLORS  BIT(0)
#define MV_PROPS   BIT(1)
#define MV_TEXPACK BIT(2)
#define MV_WEATHER BIT(3)

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
	WORLD_EXTRA_IO_RENAME,
	WORLD_EXTRA_COMPR_INIT,
	WORLD_EXTRA_COMPR_PROC
} EWorldExtra;

typedef struct _WorldInfo {
	SVec dimensions;
	BlockDef *bdefines[255];
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
	Semaphore *sem;
	Mutex *taskm;
	cs_uint32 taskc;
	cs_bool isBusy;
	cs_bool loaded;
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
#endif
