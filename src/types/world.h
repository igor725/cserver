#ifndef WORLDTYPES_H
#define WORLDTYPES_H
#include "core.h"
#include "vector.h"
#include "types/list.h"
#include "types/platform.h"
#include "types/compr.h"
#include "types/cpe.h"

#define WORLD_FLAG_NONE 0x00
#define WORLD_FLAG_LOADED BIT(0)
#define WORLD_FLAG_MODIFIED BIT(1)
#define WORLD_FLAG_MODIGNORE BIT(2)
#define WORLD_FLAG_INMEMORY BIT(3)

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
	cs_char texturepack[65];
	Vec spawnVec;
	Ang spawnAng;
	cs_int8 weatherType;
	cs_byte modval, modclr;
	cs_uint16 modprop;
} WorldInfo;

typedef struct _World {
	cs_uint32 flags;
	cs_str name;
	WorldInfo info;
	Semaphore *sem;
	Waitable *taskw;
	cs_uint32 taskc;
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
