#ifndef BLOCK_H
#define BLOCK_H
#include "world.h"

enum {
	BLOCK_AIR = 0,
	BLOCK_STONE = 1,
	BLOCK_GRASS = 2,
	BLOCK_DIRT = 3,
	BLOCK_COBBLE = 4,
	BLOCK_WOOD = 5,
	BLOCK_SAPLING = 6,
	BLOCK_BEDROCK = 7,
	BLOCK_WATER = 8,
	BLOCK_STILL_WATER = 9,
	BLOCK_LAVA = 10,
	BLOCK_STILL_LAVA = 11,
	BLOCK_SAND = 12,
	BLOCK_GRAVEL = 13,
	BLOCK_GOLD_ORE = 14,
	BLOCK_IRON_ORE = 15,
	BLOCK_COAL_ORE = 16,
	BLOCK_LOG = 17,
	BLOCK_LEAVES = 18,
	BLOCK_SPONGE = 19,
	BLOCK_GLASS = 20,
	BLOCK_RED = 21,
	BLOCK_ORANGE = 22,
	BLOCK_YELLOW = 23,
	BLOCK_LIME = 24,
	BLOCK_GREEN = 25,
	BLOCK_TEAL = 26,
	BLOCK_AQUA = 27,
	BLOCK_CYAN = 28,
	BLOCK_BLUE = 29,
	BLOCK_INDIGO = 30,
	BLOCK_VIOLET = 31,
	BLOCK_MAGENTA = 32,
	BLOCK_PINK = 33,
	BLOCK_BLACK = 34,
	BLOCK_GRAY = 35,
	BLOCK_WHITE = 36,
	BLOCK_DANDELION = 37,
	BLOCK_ROSE = 38,
	BLOCK_BROWN_SHROOM = 39,
	BLOCK_RED_SHROOM = 40,
	BLOCK_GOLD = 41,
	BLOCK_IRON = 42,
	BLOCK_DOUBLE_SLAB = 43,
	BLOCK_SLAB = 44,
	BLOCK_BRICK = 45,
	BLOCK_TNT = 46,
	BLOCK_BOOKSHELF = 47,
	BLOCK_MOSSY_ROCKS = 48,
	BLOCK_OBSIDIAN = 49,
};

enum {
	BDF_EXTENDED = BIT(0),
	BDF_DYNALLOCED = BIT(1),
	BDF_UPDATED = BIT(2),
	BDF_UNDEFINED = BIT(3)
};

enum {
	BDSOL_WALK,
	BDSOL_SWIM,
	BDSOL_SOLID
};

enum {
	BDSND_NONE,
	BDSND_WOOD,
	BDSND_GRAVEL,
	BDSND_GRASS,
	BDSND_STONE,
	BDSND_METAL,
	BDSND_GLASS,
	BDSND_WOOL,
	BDSND_SAND,
	BDSND_SNOW
};

enum {
	BDDRW_OPAQUE,
	BDDRW_TRANSPARENT,
	BDDRW_TRANSPARENT2,
	BDDRW_TRANSLUCENT,
	BDDRW_GAS
};

typedef struct {
	BlockID id;
	cs_str name;
	cs_byte flags;
	union {
		struct _BlockParamsExt {
			cs_byte solidity;
			cs_byte moveSpeed;
			cs_byte topTex, leftTex;
			cs_byte rightTex, frontTex;
			cs_byte backTex, bottomTex;
			cs_byte transmitsLight;
			cs_byte walkSound;
			cs_byte fullBright;
			cs_byte minX, minY, minZ;
			cs_byte maxX, maxY, maxZ;
			cs_byte blockDraw;
			cs_byte fogDensity;
			cs_byte fogR, fogG, fogB;
		} ext;
		struct _BlockParams {
			cs_byte solidity;
			cs_byte moveSpeed;
			cs_byte topTex, sideTex, bottomTex;
			cs_byte transmitsLight;
			cs_byte walkSound;
			cs_byte fullBright;
			cs_byte shape;
			cs_byte blockDraw;
			cs_byte fogDensity;
			cs_byte fogR, fogG, fogB;
		} nonext;
	} params;
} BlockDef;

typedef struct {
	World *world;
	cs_bool autosend;
	struct _BBUData {
		cs_byte count;
		cs_byte offsets[1024];
		BlockID ids[256];
	} data;
} BulkBlockUpdate;

BlockDef *Block_DefinitionsList[255];
API cs_bool Block_IsValid(BlockID id);
API cs_str Block_GetName(BlockID id);

API BlockDef *Block_New(BlockID id, cs_str name, cs_byte flags);
API cs_bool Block_Define(BlockDef *bdef);
API cs_bool Block_Undefine(BlockID id);
API void Block_UpdateDefinitions();

API cs_bool Block_BulkUpdateAdd(BulkBlockUpdate *bbu, cs_uint32 offset, BlockID id);
API void Block_BulkUpdateSend(BulkBlockUpdate *bbu);
API void Block_BulkUpdateClean(BulkBlockUpdate *bbu);
#endif // BLOCK_H
