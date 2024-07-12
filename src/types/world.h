#ifndef WORLDTYPES_H
#define WORLDTYPES_H
#include "core.h"
#include "vector.h"
#include "types/list.h"
#include "types/platform.h"
#include "types/compr.h"
#include "types/cpe.h"

#define WORLD_FLAG_NONE 0x00
#define WORLD_FLAG_ALLOCATED BIT(0)
#define WORLD_FLAG_MODIFIED BIT(1)
#define WORLD_FLAG_MODIGNORE BIT(2)
#define WORLD_FLAG_INMEMORY BIT(3)

#define WORLD_PROC_ALL ((WorldProcs)-1)
#define WORLD_PROC_LOADING 0
#define WORLD_PROC_SAVING 1
#define WORLD_PROC_SENDWORLD 2

#define WORLD_MAX_SIZE 4000000000u
#define WORLD_INVALID_OFFSET (cs_uint32)-1

typedef cs_uint16 WorldFlags;
typedef cs_uint16 WorldProcs;

typedef enum _EWorldError {
	WORLD_ERROR_SUCCESS = 0,
	WORLD_ERROR_IOFAIL,
	WORLD_ERROR_COMPR,
	WORLD_ERROR_INFOREAD,
	WORLD_ERROR_DATAREAD,
	WORLD_ERROR_INMEMORY
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
	BlockDef *bdefines[256];
	Color3 colors[WORLD_COLORS_COUNT];
	cs_int32 props[WORLD_PROPS_COUNT];
	cs_char texturepack[MAX_STR_LEN];
	Vec spawnVec;
	Ang spawnAng;
	cs_int8 weatherType;
	cs_byte modval, modclr;
	cs_uint16 modprop;
	cs_uint32 seed;
} WorldInfo;

typedef struct _World {
	WorldFlags flags;
	WorldProcs processes;
	cs_byte prCount[sizeof(WorldProcs) * 8];
	cs_str name;
	WorldInfo info;
	Mutex *mtx;
	Waitable *taskann;
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
