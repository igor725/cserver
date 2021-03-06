#include "core.h"
#include "world.h"
#include "block.h"
#include "csmath.h"
#include "platform.h"
#include "generators.h"

#define GENROUTINE(N) static void generate##N(void)
#define setBlock(vec, id) ctx.data[(vec.y * ctx.dims->z + vec.z) * ctx.dims->x + vec.x] = id
#define getBlock(vec) ctx.data[(vec.y * ctx.dims->z + vec.z) * ctx.dims->x + vec.x]

static const cs_bool gen_enable_caves = true,
gen_enable_trees = true, gen_enable_ores = true;

static const cs_uint16 gen_cave_radius = 3,
gen_cave_min_length = 100, gen_cave_max_length = 500,
gen_ore_vein_min_size = 4, gen_ore_vein_max_size = 9,
gen_gravel_vein_size = 14, gen_biome_step = 20,
gen_biome_radius = 5;

static const cs_float gen_trees_count_mult = 1.0f / 250.0f,
gen_ores_count_mult = 1.0f / 800.0f,
gen_caves_count_mult = 3.2f / 200000.0f;

static struct {
	RNGState rnd;
	BlockID *data;
	SVec *dims;
	cs_uint32 planeSize, biomesNum,
	biomeSize, numCaves, worldSize;
	cs_uint16 *biomes, *heightMap,
	*biomesWithTrees, biomeSizeX,
	biomeSizeZ, heightGrass,
	heightWater, heightStone, heightLava,
	gravelVeinSize;
} ctx;

typedef enum _ENGenBiomes {
	NGEN_BIOME_INVALID,
	NGEN_BIOME_NORMAL,
	NGEN_BIOME_HIGH,
	NGEN_BIOME_TREES,
	NGEN_BIOME_SAND,
	NGEN_BIOME_WATER
} ENGenBiomes;

static void genBiomesAndHeightmap(void) {
	ctx.biomeSizeX = (ctx.dims->x / gen_biome_step) + 2;
	ctx.biomeSizeZ = (ctx.dims->z / gen_biome_step) + 2;
	ctx.biomeSize = ctx.biomeSizeX * ctx.biomeSizeZ;
	ctx.biomesNum = ctx.planeSize / gen_biome_step / gen_biome_radius / 64;
	ctx.biomes = Memory_Alloc(2, ctx.biomeSize);
	ctx.heightMap = Memory_Alloc(2, ctx.biomeSize);
	for(cs_uint16 i = 0; i < ctx.biomeSize; i++)
		ctx.biomes[i] = NGEN_BIOME_NORMAL;

	for(cs_uint32 i = 0; i < ctx.biomesNum; i++) {
		cs_uint16 x = (cs_uint16)Random_Next(&ctx.rnd, ctx.biomeSizeX),
		z = (cs_uint16)Random_Next(&ctx.rnd, ctx.biomeSizeZ),
		biome = (cs_uint16)Random_Range(&ctx.rnd, NGEN_BIOME_NORMAL, NGEN_BIOME_WATER);

		for(cs_int16 dx = -gen_biome_radius; dx <= gen_biome_radius; dx++) {
			for(cs_int16 dz = -gen_biome_radius; dz <= gen_biome_radius; dz++) {
				cs_int16 nx = x + dx,
				nz = z + dz;

				if(dx * dx + dz * dz < (gen_biome_radius * gen_biome_radius) &&
				0 <= nx && nx < ctx.biomeSizeX &&
				0 <= nz && nz < ctx.biomeSizeZ) {
					cs_uint16 offset = nx + nz * ctx.biomeSizeX;
					if(offset < ctx.biomeSize)
						ctx.biomes[offset] = biome;
				}
			}
		}
	}

	for(cs_uint16 x = 0; x < ctx.biomeSizeX; x++) {
		for(cs_uint16 z = 0; z < ctx.biomeSizeZ; z++) {
			cs_uint16 offset = x + z * ctx.biomeSizeX,
			biome = ctx.biomes[offset];

			switch (biome) {
				case NGEN_BIOME_NORMAL:
					if(Random_Next(&ctx.rnd, 6) == 0)
						ctx.heightMap[offset] = ctx.heightGrass + (cs_uint16)Random_Range(&ctx.rnd, -3, -1);
					else
						ctx.heightMap[offset] = ctx.heightGrass + (cs_uint16)Random_Range(&ctx.rnd, 1, 3);
					break;
				case NGEN_BIOME_HIGH:
					if(Random_Next(&ctx.rnd, 30) == 0)
						ctx.heightMap[offset] = ctx.heightGrass +
						(cs_uint16)Random_Range(&ctx.rnd, 20, min(ctx.dims->y - ctx.heightGrass - 1, 40));
					else
						ctx.heightMap[offset] = ctx.heightGrass + (cs_int16)Random_Range(&ctx.rnd, -2, 20);
					break;
				case NGEN_BIOME_TREES:
					ctx.heightMap[offset] = ctx.heightGrass + (cs_uint16)Random_Range(&ctx.rnd, 1, 5);
					break;
				case NGEN_BIOME_SAND:
					ctx.heightMap[offset] = ctx.heightGrass + (cs_uint16)Random_Range(&ctx.rnd, 1, 4);
					break;
				case NGEN_BIOME_WATER:
					if(Random_Next(&ctx.rnd, 10) == 0)
						ctx.heightMap[offset] = ctx.heightGrass + (cs_int16)Random_Range(&ctx.rnd, -20, -3);
					else
						ctx.heightMap[offset] = ctx.heightGrass + (cs_int16)Random_Range(&ctx.rnd, -10, -3);
					break;
				case NGEN_BIOME_INVALID:
					ctx.heightMap[offset] = ctx.heightGrass;
			}

			ctx.heightMap[offset] = max(min(ctx.heightMap[offset], ctx.dims->y - 2), 3);
		}
	}
}

INL static cs_uint16 getHeight(cs_uint16 x, cs_uint16 z) {
	cs_uint16 hx = x / gen_biome_step, hz = z / gen_biome_step;
	float percentX = (cs_float)x / (cs_float)gen_biome_step - hx,
	percentZ = (cs_float)z / (cs_float)gen_biome_step - hz;

	return (cs_uint16)(((cs_float)ctx.heightMap[hx + hz * ctx.biomeSizeX] * (1.0f - percentX)
	+ (cs_float)ctx.heightMap[(hx + 1) + hz * ctx.biomeSizeX] * percentX)
	* (1.0f - percentZ) + ((cs_float)ctx.heightMap[hx + (hz + 1) * ctx.biomeSizeX]
	* (1.0f - percentX) + (cs_float)ctx.heightMap[(hx + 1) + (hz + 1) * ctx.biomeSizeX] * percentX)
	* percentZ + 0.5f);
}

GENROUTINE(Terrain) {
	cs_uint16 height1, heightStone1;
	ENGenBiomes biome = NGEN_BIOME_INVALID;
	for(cs_uint16 x = 0; x < ctx.dims->x; x++) {
		cs_uint16 hx = x / gen_biome_step,
		biomePosZOld = (cs_uint16)-1,
		b0 = hx,
		b1 = b0 + 1,
		b00 = NGEN_BIOME_INVALID,
		b01 = ctx.biomes[b0],
		b10 = NGEN_BIOME_INVALID,
		b11 = ctx.biomes[b1];
		cs_float percentPosX = (cs_float)x / (cs_float)gen_biome_step - (cs_float)hx,
		percentNegX = 1.0f - percentPosX;
		for(cs_uint16 z = 0; z < ctx.dims->z; z++) {
			cs_uint16 hz = z / gen_biome_step;
			cs_float percentZ = (cs_float)z / (cs_float)gen_biome_step - (cs_float)hz,
			percentZOp = 1.0f - percentZ;
			height1 = (cs_uint16)(((cs_float)ctx.heightMap[hx + hz * ctx.biomeSizeX] * percentNegX +
			(cs_float)ctx.heightMap[(hx + 1) + hz * ctx.biomeSizeX] * percentPosX) * percentZOp +
			((cs_float)ctx.heightMap[hx + (hz + 1) * ctx.biomeSizeX] * percentNegX +
			(cs_float)ctx.heightMap[(hx + 1) + (hz + 1) * ctx.biomeSizeX] * percentPosX) *
			percentZ + 0.5f);
			height1 = min(height1, ctx.dims->y - 1);
			heightStone1 = min(max(height1 - (cs_uint16)Random_Range(&ctx.rnd, 4, 6), 1), ctx.dims->y - 1);

			if(hz != biomePosZOld) {
				biomePosZOld = hz;
				b00 = b01;
				b01 = ctx.biomes[b0 + (hz + 1) * ctx.biomeSizeX];
				b10 = b11;
				b11 = ctx.biomes[b1 + (hz + 1) * ctx.biomeSizeX];
				if(b01 == NGEN_BIOME_TREES) b01 = NGEN_BIOME_NORMAL;
				if(b11 == NGEN_BIOME_TREES) b11 = NGEN_BIOME_NORMAL;
			}

			if(b11 == b01 && b11 == b10) {
				if(percentPosX * percentPosX + percentZ * percentZ > 0.25f)
					biome = b11;
				else
					biome = b00;
			} else if(b00 == b11 && b00 == b10) {
				if(percentPosX * percentPosX + percentZOp * percentZOp > 0.25f)
					biome = b00;
				else
					biome = b01;
			} else if(b00 == b01 && b00 == b11) {
				if(percentNegX * percentNegX + percentZ * percentZ > 0.25f)
					biome = b00;
				else
					biome = b10;
			} else if(b00 == b01 && b00 == b10) {
				if(percentNegX * percentNegX + percentZOp * percentZOp > 0.25f)
					biome = b00;
				else
					biome = b11;
			} else {
				cs_uint16 bx = (cs_uint16)(hx + 0.5f),
				bz = (cs_uint16)(hz + 0.5f);
				bx = min(bx, ctx.biomeSizeX);
				bz = min(bz, ctx.biomeSizeZ);
				biome = ctx.biomes[bx + bz * ctx.biomeSizeX];
			}

			cs_uint32 offset = z * ctx.dims->x + x;
			cs_uint16 y; // For iterators

			for(y = ctx.heightStone; y < heightStone1; y++)
				ctx.data[offset + y * ctx.planeSize] = BLOCK_STONE;

			switch (biome) {
				case NGEN_BIOME_NORMAL:
				case NGEN_BIOME_TREES:
					for(y = heightStone1; y < height1 - 1; y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_DIRT;

					if(height1 > ctx.heightWater) {
						ctx.data[offset + (height1 - 1) * ctx.planeSize] = BLOCK_DIRT;
						ctx.data[offset + height1 * ctx.planeSize] = BLOCK_GRASS;
					} else {
						ctx.data[offset + (height1 - 1) * ctx.planeSize] = BLOCK_DIRT;
						ctx.data[offset + height1 * ctx.planeSize] = BLOCK_SAND;
					}

					for(y = height1 + 1; y <= ctx.heightWater; y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_WATER;
					break;
				case NGEN_BIOME_HIGH:
					for(y = heightStone1; y <= height1; y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_STONE;
					for(y = height1 + 1; y <= ctx.heightWater; y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_WATER;
					break;
				case NGEN_BIOME_SAND:
					for(y = heightStone1; y <= height1; y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_SAND;
					for(y = height1 + 1; y <= ctx.heightWater; y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_WATER;
					break;
				case NGEN_BIOME_WATER:
					for(y = heightStone1; y <= min(height1, ctx.heightGrass - 2); y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_DIRT;
					for(y = max(heightStone1, ctx.heightGrass - 2); y <= height1; y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_SAND;
					for(y = height1 + 1; y <= ctx.heightWater; y++)
						ctx.data[offset + y * ctx.planeSize] = BLOCK_WATER;
					break;
				case NGEN_BIOME_INVALID:
					ctx.data[offset + (height1 - 1) * ctx.planeSize] =
					(BlockID)Random_Range(&ctx.rnd, BLOCK_RED, BLOCK_BLACK);
					ctx.data[offset + height1 * ctx.planeSize] =
					(BlockID)Random_Range(&ctx.rnd, BLOCK_RED, BLOCK_BLACK);
					break;
			}
		}
	}
}

GENROUTINE(Cave) {
	cs_uint16 caveLength = (cs_uint16)Random_Range(&ctx.rnd, gen_cave_min_length, gen_cave_max_length);

	SVec pos;
	Vec delta, direction;

	pos.x = (cs_int16)Random_Range(&ctx.rnd, gen_cave_radius, ctx.dims->x - gen_cave_radius);
	pos.y = (cs_int16)Random_Range(&ctx.rnd, 10, ctx.heightGrass - 20);
	pos.z = (cs_int16)Random_Range(&ctx.rnd, gen_cave_radius, ctx.dims->z - gen_cave_radius);

	direction.x = (Random_Float(&ctx.rnd) - 0.5f) * 0.6f;
	direction.y = -Random_Float(&ctx.rnd) * 0.1f;
	direction.z = (Random_Float(&ctx.rnd) - 0.5f) * 0.6f;

	for(cs_uint16 j = 1; j <= caveLength; j++) {
		if(j % (caveLength / 3) == 0) {
			direction.x = (Random_Float(&ctx.rnd) - 0.5f) * 0.6f;
			direction.y = (Random_Float(&ctx.rnd) - 0.5f) * 0.2f;
			direction.z = (Random_Float(&ctx.rnd) - 0.5f) * 0.6f;
		}

		delta.x = Random_Float(&ctx.rnd) - 0.5f + direction.x;
		delta.y = (Random_Float(&ctx.rnd) - 0.5f) * 0.4f + direction.y;
		delta.z = Random_Float(&ctx.rnd) - 0.5f + direction.z;

		cs_float length = Math_Sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
		pos.x = (cs_int16)((cs_float)pos.x + delta.x * gen_cave_radius / length + 0.5f);
		pos.y = (cs_int16)((cs_float)pos.y + delta.y * gen_cave_radius / length + 0.5f);
		pos.z = (cs_int16)((cs_float)pos.z + delta.z * gen_cave_radius / length + 0.5f);

		SVec bpos;
		for(cs_int16 dx = -gen_cave_radius; dx <= gen_cave_radius; dx++) {
			for(cs_int16 dz = -gen_cave_radius; dz <= gen_cave_radius; dz++) {
				for(cs_int16 dy = -gen_cave_radius; dy <= gen_cave_radius; dy++) {
					bpos.x = pos.x + dx;
					bpos.y = pos.y + dy;
					bpos.z = pos.z + dz;

					if(dx * dx + dz * dz + dy * dy < (gen_cave_radius * gen_cave_radius)
					&& 1 < bpos.y && bpos.y < ctx.dims->y - 2
					&& 1 < bpos.x && bpos.x < ctx.dims->x - 2
					&& 1 < bpos.z && bpos.z < ctx.dims->z - 2) {
						BlockID curr = getBlock(bpos);
						if(curr < BLOCK_WATER || curr > BLOCK_WATER_STILL) {
							setBlock(bpos, bpos.y > ctx.heightLava ? BLOCK_AIR : BLOCK_LAVA);
						} else {
							bpos.y -= 1;
							setBlock(bpos, BLOCK_WATER);
						}
					}
				}
			}
		}
	}
}

GENROUTINE(Ores) {
	cs_uint32 oreCount = (cs_uint32)(ctx.worldSize * gen_ores_count_mult);

	SVec pos, tmp;
	for(; oreCount > 0; oreCount--) {
		cs_uint16 vein_size = (cs_uint16)Random_Range(&ctx.rnd, gen_ore_vein_min_size, gen_ore_vein_max_size);
		pos.x = (cs_int16)Random_Range(&ctx.rnd, 0, ctx.dims->x - vein_size);
		pos.z = (cs_int16)Random_Range(&ctx.rnd, 0, ctx.dims->z - vein_size);
		pos.y = (cs_int16)(3.0f * Random_Float(&ctx.rnd) *
		(cs_float)min(ctx.dims->y - vein_size, ctx.heightGrass + 15));

		BlockID id = (BlockID)Random_Range(&ctx.rnd, BLOCK_GOLD_ORE, BLOCK_COAL_ORE);
		for(tmp.x = -vein_size; tmp.x < vein_size; tmp.x++) {
			for(tmp.y = -vein_size; tmp.y < vein_size; tmp.y++) {
				for(tmp.z = -vein_size; tmp.z < vein_size; tmp.z++) {
					pos.x += tmp.x; pos.y += tmp.y; pos.z += tmp.z;
					if(pos.x < ctx.dims->x && pos.y < ctx.dims->y && pos.z < ctx.dims->z &&
					pos.x > 0 && pos.y > 0 && pos.z > 0)
						if(getBlock(pos) == 1) setBlock(pos, id);
				}
			}
		}
	}
}

static void placeTree(SVec treePos) {
	cs_uint16 baseHeight = treePos.y;
	treePos.y += (cs_uint16)Random_Range(&ctx.rnd, 4, 6);
	for(cs_uint16 dz = treePos.z - 2; dz <= treePos.z + 2; dz++) {
		for(cs_uint16 y = treePos.y - 2; y <= treePos.y - 1; y++) {
			cs_uint32 offset = (y * ctx.dims->z + dz) * ctx.dims->x + treePos.x - 2;
			Memory_Fill(ctx.data + offset, 5, BLOCK_LEAVES);
		}
	}

	SVec tmp = treePos;
	for(tmp.y = baseHeight + 1; tmp.y <= treePos.y; tmp.y++)
		setBlock(tmp, BLOCK_LOG);

	tmp = treePos;
	for(tmp.x = treePos.x - 1; tmp.x <= treePos.x + 1; tmp.x++)
		if(tmp.x != treePos.x) for(tmp.y = treePos.y; tmp.y < treePos.y + 1; tmp.y++)
			setBlock(tmp, BLOCK_LEAVES);

	tmp = treePos;
	for(tmp.z = treePos.z - 1; tmp.z <= treePos.z + 1; tmp.z++)
		if(tmp.z != treePos.z) for(tmp.y = treePos.y; tmp.y <= treePos.y + 1; tmp.y++)
			setBlock(tmp, BLOCK_LEAVES);

	tmp = treePos; tmp.y++;
	setBlock(tmp, BLOCK_LEAVES);
}

GENROUTINE(Trees) {
	ctx.biomesWithTrees = Memory_Alloc(2, ctx.biomeSize);

	cs_uint16 biomesWithTreesNum = 0;
	for(cs_uint16 i = 0; i < ctx.biomeSize; i++) {
		ENGenBiomes biome = ctx.biomes[i];
		if(biome == NGEN_BIOME_TREES || biome == NGEN_BIOME_SAND)
			ctx.biomesWithTrees[biomesWithTreesNum++] = i;
	}

	cs_uint32 trees = ctx.planeSize;
	trees = (cs_uint32)((cs_float)trees * gen_trees_count_mult);
	trees = (cs_uint32)((cs_float)trees * ((cs_float)biomesWithTreesNum / ctx.biomeSize));

	for(cs_uint32 i = 0; i < trees; i++) {
		cs_uint16 randBiome = (cs_uint16)Random_Range(&ctx.rnd, 1, biomesWithTreesNum);
		SVec treePos;
		treePos.x = (ctx.biomesWithTrees[randBiome] % ctx.biomeSizeX)
			* gen_biome_step + (cs_uint16)Random_Range(&ctx.rnd, 1, gen_biome_step)
			- gen_biome_step / 2;
		treePos.z = (cs_uint16)(((cs_float)ctx.biomesWithTrees[randBiome] / (cs_float)ctx.biomeSizeX)
			* gen_biome_step + (cs_uint16)Random_Range(&ctx.rnd, 1, gen_biome_step) - gen_biome_step / 2);

		treePos.x = max(6, min(ctx.dims->x - 6, treePos.x));
		treePos.z = max(6, min(ctx.dims->z - 6, treePos.z));

		treePos.y = getHeight(treePos.x, treePos.z);
		cs_uint16 cactusHeight = treePos.y;
		if(treePos.y > ctx.heightWater && treePos.y + 8 < ctx.dims->y) {
			ENGenBiomes biome = ctx.biomes[ctx.biomesWithTrees[randBiome]];

			switch (biome) {
				case NGEN_BIOME_TREES:
					placeTree(treePos);
					break;
				case NGEN_BIOME_SAND:
					for(cactusHeight += (cs_uint16)Random_Range(&ctx.rnd, 1, 4); treePos.y <= cactusHeight; treePos.y++)
						setBlock(treePos, BLOCK_LEAVES);
					break;
				case NGEN_BIOME_INVALID:
				case NGEN_BIOME_NORMAL:
				case NGEN_BIOME_HIGH:
				case NGEN_BIOME_WATER:
					break;
			}
		}
	}
}

INL static cs_float getHeightAt(cs_int16 x, cs_int16 z) {
	cs_int16 y;

	for(y = ctx.dims->y; y > 0;) {
		cs_uint32 offset = (--y * ctx.dims->z + z) * ctx.dims->x + x;
		if(ctx.data[offset] != BLOCK_AIR) break;
	}

	return (cs_float)y + 2.59375f;
}

cs_bool normalgenerator(World *world, cs_uint32 seed) {
	if(world->info.dimensions.x < 32 ||
	world->info.dimensions.z < 32 ||
	world->info.dimensions.y < 32)
		return false;

	Memory_Zero(&ctx, sizeof(ctx));
	if(seed == GENERATOR_SEED_FROM_TIME)
		seed = (cs_uint32)(Time_GetMSec() / 1000ull);

	Random_Seed(&ctx.rnd, seed);
	ctx.dims = &world->info.dimensions;
	ctx.planeSize = ctx.dims->x * ctx.dims->z;
	ctx.data = World_GetBlockArray(world, &ctx.worldSize);

	ctx.heightLava = 7;
	ctx.heightGrass = ctx.dims->y / 2;
	ctx.heightWater = ctx.heightGrass;
	ctx.heightStone = ctx.heightGrass - 3;

	ctx.gravelVeinSize = min(gen_gravel_vein_size, ctx.heightGrass / 3);
	ctx.numCaves = (cs_uint16)((cs_float)(ctx.planeSize * ctx.heightGrass) * gen_caves_count_mult);

	genBiomesAndHeightmap();
	Memory_Fill(ctx.data, ctx.planeSize, BLOCK_BEDROCK);
	Memory_Fill(ctx.data + ctx.planeSize, ctx.planeSize * (ctx.heightStone - 1), BLOCK_STONE);

	generateTerrain();
	if(gen_enable_ores)
		generateOres();
	if(gen_enable_trees)
		generateTrees();
	for(cs_uint16 i = 0; i < ctx.numCaves && gen_enable_caves; i++)
		generateCave();

	WorldInfo *wi = &world->info;
	cs_int16 x = ctx.dims->x / 2, z = ctx.dims->z / 2;
	Vec_Set(wi->spawnVec, x, getHeightAt(x, z), z);
	World_SetEnvProp(world, WORLD_PROP_SIDEBLOCK, BLOCK_AIR);
	World_SetEnvProp(world, WORLD_PROP_EDGEBLOCK, BLOCK_WATER);
	World_SetEnvProp(world, WORLD_PROP_CLOUDSLEVEL, ctx.dims->y + 2);
	World_SetEnvProp(world, WORLD_PROP_EDGELEVEL, ctx.heightWater + 1);
	World_SetEnvProp(world, WORLD_PROP_SIDEOFFSET, 0);
	World_SetSeed(world, seed);

	if(ctx.biomes) Memory_Free(ctx.biomes);
	if(ctx.heightMap) Memory_Free(ctx.heightMap);
	if(ctx.biomesWithTrees) Memory_Free(ctx.biomesWithTrees);

	return true;
}
