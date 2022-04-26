#include "core.h"
#include "str.h"
#include "tests.h"
#include "block.h"
#include "vector.h"
#include "world.h"

#define COMPARE_COLORS(c1, c2) ((c1).r == (c2).r || (c1).g == (c2).g || (c1).b == (c2).b)

cs_bool Tests_World(void) {
	Tests_NewTask("Create world");
	cs_str worldname = "__test";
	World *world = World_Create(worldname);
	Tests_Assert(world != NULL, "create world structure");
	SVec dims = {1024, 64, 1024};
	World_SetDimensions(world, &dims);
	World_AllocBlockArray(world);
	cs_uint32 wsize = 0,
	ewsize = (cs_uint32)dims.x * (cs_uint32)dims.y * (cs_uint32)dims.z;
	Tests_Assert(World_GetBlockArray(world, &wsize) != NULL, "check block array");
	Tests_Assert(wsize == ewsize, "check world data array size");
	Tests_Assert(String_CaselessCompare(World_GetName(world), worldname), "check world name");

	Tests_NewTask("Change world properties");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_SIDEBLOCK, BLOCK_DIRT), "set side block");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_EDGEBLOCK, BLOCK_GRASS), "set edge block");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_EDGELEVEL, 40), "set edge level");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_CLOUDSLEVEL, 228), "set clouds level");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_FOGDIST, 10), "set fog distance");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_SPDCLOUDS, 100), "set clouds speed");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_SPDWEATHER, 250), "set weather speed");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_FADEWEATHER, 250), "set weather fade");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_EXPFOG, 1), "set exponential fog");
	Tests_Assert(World_SetEnvProp(world, WORLD_PROP_SIDEOFFSET, -6), "set map sides offset");
	Tests_Assert(World_SetTexturePack(world, "http://test.texture/pack.zip"), "set texture pack");
	Tests_Assert(World_SetWeather(world, WORLD_WEATHER_SNOW), "set weather");
	Color3 skycol = {1, 2, 3}, cloudcol = {4, 5, 6},
	fogcol = {7, 8, 9}, ambcol = {4, 5, 6}, diffcol = {4, 5, 6};
	Tests_Assert(World_SetEnvColor(world, WORLD_COLOR_SKY, &skycol), "set env color");
	Tests_Assert(World_SetEnvColor(world, WORLD_COLOR_CLOUD, &cloudcol), "set cloud color");
	Tests_Assert(World_SetEnvColor(world, WORLD_COLOR_FOG, &fogcol), "set fog color");
	Tests_Assert(World_SetEnvColor(world, WORLD_COLOR_AMBIENT, &ambcol), "set ambient color");
	Tests_Assert(World_SetEnvColor(world, WORLD_COLOR_DIFFUSE, &diffcol), "set diffuse color");

	Tests_NewTask("Place blocks in world");
	SVec p1 = {.x = 1, .y = 3, .z = 3},
	p2 = {.x = 2, .y = 2, .z = 8},
	p3 = {.x = dims.x, .y = 0, .z = 0};
	Tests_Assert(World_SetBlock(world, &p1, BLOCK_BEDROCK), "set first block");
	Tests_Assert(World_SetBlock(world, &p2, BLOCK_LOG), "set second block");
	Tests_Assert(World_SetBlockO(world, wsize - 1, BLOCK_WATER_STILL), "set third block by offset");
	Tests_Assert(World_SetBlockO(world, wsize, BLOCK_WATER) == false, "set fourth block outside of world");
	Tests_Assert(World_SetBlock(world, &p3, BLOCK_DIRT) == false, "set fifth block");

	Tests_NewTask("Saving world");
	Tests_Assert(World_Save(world), "unload world");
	World_Lock(world, 0);
	World_Unlock(world);
	World_Free(world);

	Tests_NewTask("Load world");
	world = World_Create(worldname);
	Tests_Assert(world != NULL, "create world structure");
	Tests_Assert(World_Load(world), "load world");
	World_Lock(world, 0);
	World_Unlock(world);

	Tests_NewTask("Checking world properties");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_SIDEBLOCK) == BLOCK_DIRT, "check side block");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_EDGEBLOCK) == BLOCK_GRASS, "check edge block");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_EDGELEVEL) == 40, "check edge level");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_CLOUDSLEVEL) == 228, "check clouds level");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_FOGDIST) == 10, "check fog distance");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_SPDCLOUDS) == 100, "check clouds speed");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_SPDWEATHER) == 250, "check weather speed");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_FADEWEATHER) == 250, "check weather fade");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_EXPFOG) == 1, "check exponential fog");
	Tests_Assert(World_GetEnvProp(world, WORLD_PROP_SIDEOFFSET) == -6, "check map sides offset");
	Tests_Assert(String_Compare(World_GetTexturePack(world), "http://test.texture/pack.zip"), "check texture pack");
	Tests_Assert(World_GetWeather(world) == WORLD_WEATHER_SNOW, "check weather");
	Color3 cskycol, cfogcol, ccloudcol, cambcol, cdiffcol;
	Tests_Assert(World_GetEnvColor(world, WORLD_COLOR_SKY, &cskycol), "reading sky color");
	Tests_Assert(World_GetEnvColor(world, WORLD_COLOR_FOG, &cfogcol), "reading fog color");
	Tests_Assert(World_GetEnvColor(world, WORLD_COLOR_CLOUD, &ccloudcol), "reading cloud color");
	Tests_Assert(World_GetEnvColor(world, WORLD_COLOR_AMBIENT, &cambcol), "reading ambient color");
	Tests_Assert(World_GetEnvColor(world, WORLD_COLOR_DIFFUSE, &cdiffcol), "reading diffuse color");
	Tests_Assert(COMPARE_COLORS(cskycol, skycol), "check sky color");
	Tests_Assert(COMPARE_COLORS(cfogcol, fogcol), "check fog color");
	Tests_Assert(COMPARE_COLORS(ccloudcol, cloudcol), "check cloud color");
	Tests_Assert(COMPARE_COLORS(cambcol, ambcol), "check ambient color");
	Tests_Assert(COMPARE_COLORS(cdiffcol, diffcol), "check diffuse color");

	Tests_NewTask("Check blocks placing");
	Tests_Assert(World_GetBlock(world, &p1) == BLOCK_BEDROCK, "check first block");
	Tests_Assert(World_GetBlock(world, &p2) == BLOCK_LOG, "check second block");
	Tests_Assert(World_GetBlockO(world, wsize - 1) == BLOCK_WATER_STILL, "check third block by offset");
	Tests_Assert(World_GetBlockO(world, wsize) == (BlockID)-1, "check block outside world");
	World_FreeBlockArray(world);
	World_Free(world);

	return true;
}
