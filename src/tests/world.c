#include "core.h"
#include "str.h"
#include "tests.h"
#include "block.h"
#include "vector.h"
#include "world.h"

#define COMPARE_COLORS(c1, c2) ((c1)->r == (c2)->r || (c1)->g == (c2)->g || (c1)->b == (c2)->b)

cs_bool Tests_World(void) {
	Tests_NewTask("Create world");
	cs_str worldname = "__test.cws";
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
	Tests_Assert(World_SetProperty(world, PROP_SIDEBLOCK, BLOCK_DIRT), "set side block");
	Tests_Assert(World_SetProperty(world, PROP_EDGEBLOCK, BLOCK_GRASS), "set edge block");
	Tests_Assert(World_SetProperty(world, PROP_EDGELEVEL, 40), "set edge level");
	Tests_Assert(World_SetProperty(world, PROP_CLOUDSLEVEL, 228), "set clouds level");
	Tests_Assert(World_SetProperty(world, PROP_FOGDIST, 10), "set fog distance");
	Tests_Assert(World_SetProperty(world, PROP_SPDCLOUDS, 100), "set clouds speed");
	Tests_Assert(World_SetProperty(world, PROP_SPDWEATHER, 250), "set weather speed");
	Tests_Assert(World_SetProperty(world, PROP_FADEWEATHER, 250), "set weather fade");
	Tests_Assert(World_SetProperty(world, PROP_EXPFOG, 1), "set exponential fog");
	Tests_Assert(World_SetProperty(world, PROP_SIDEOFFSET, -6), "set map sides offset");
	Tests_Assert(World_SetTexturePack(world, "http://test.texture/pack.zip"), "set texture pack");
	Tests_Assert(World_SetWeather(world, WEATHER_SNOW), "set weather");
	Color3 skycol = {1, 2, 3}, cloudcol = {4, 5, 6},
	fogcol = {7, 8, 9}, ambcol = {4, 5, 6}, diffcol = {4, 5, 6};
	Tests_Assert(World_SetEnvColor(world, COLOR_SKY, &skycol), "set env color");
	Tests_Assert(World_SetEnvColor(world, COLOR_CLOUD, &cloudcol), "set cloud color");
	Tests_Assert(World_SetEnvColor(world, COLOR_FOG, &fogcol), "set fog color");
	Tests_Assert(World_SetEnvColor(world, COLOR_AMBIENT, &ambcol), "set ambient color");
	Tests_Assert(World_SetEnvColor(world, COLOR_DIFFUSE, &diffcol), "set diffuse color");

	Tests_NewTask("Place blocks in world");
	SVec p1 = {.x = 1, .y = 3, .z = 3},
	p2 = {.x = 2, .y = 2, .z = 8};
	Tests_Assert(World_SetBlock(world, &p1, BLOCK_BEDROCK), "set first block");
	Tests_Assert(World_SetBlock(world, &p2, BLOCK_LOG), "set second block");
	Tests_Assert(World_SetBlockO(world, wsize - 1, BLOCK_WATER_STILL), "set third block by offset");
	Tests_Assert(World_SetBlockO(world, wsize, BLOCK_WATER) == false, "set fourth block outside of world");

	Tests_NewTask("Saving world");
	Tests_Assert(World_Save(world, true), "unload world");
	World_Free(world);

	Tests_NewTask("Load world");
	world = World_Create(worldname);
	Tests_Assert(world != NULL, "create world structure");
	Tests_Assert(World_Load(world), "load world");
	Waitable_Wait(world->waitable);

	Tests_NewTask("Checking world properties");
	Tests_Assert(World_GetProperty(world, PROP_SIDEBLOCK) == BLOCK_DIRT, "check side block");
	Tests_Assert(World_GetProperty(world, PROP_EDGEBLOCK) == BLOCK_GRASS, "check edge block");
	Tests_Assert(World_GetProperty(world, PROP_EDGELEVEL) == 40, "check edge level");
	Tests_Assert(World_GetProperty(world, PROP_CLOUDSLEVEL) == 228, "check clouds level");
	Tests_Assert(World_GetProperty(world, PROP_FOGDIST) == 10, "check fog distance");
	Tests_Assert(World_GetProperty(world, PROP_SPDCLOUDS) == 100, "check clouds speed");
	Tests_Assert(World_GetProperty(world, PROP_SPDWEATHER) == 250, "check weather speed");
	Tests_Assert(World_GetProperty(world, PROP_FADEWEATHER) == 250, "check weather fade");
	Tests_Assert(World_GetProperty(world, PROP_EXPFOG) == 1, "check exponential fog");
	Tests_Assert(World_GetProperty(world, PROP_SIDEOFFSET) == -6, "check map sides offset");
	Tests_Assert(String_Compare(World_GetTexturePack(world), "http://test.texture/pack.zip"), "check texture pack");
	Tests_Assert(World_GetWeather(world) == WEATHER_SNOW, "check weather");
	Color3 *cskycol = World_GetEnvColor(world, COLOR_SKY),
	*cfogcol = World_GetEnvColor(world, COLOR_FOG),
	*ccloudcol = World_GetEnvColor(world, COLOR_CLOUD),
	*cambcol = World_GetEnvColor(world, COLOR_AMBIENT),
	*cdiffcol = World_GetEnvColor(world, COLOR_DIFFUSE);
	Tests_Assert(COMPARE_COLORS(cskycol, &skycol), "check sky color");
	Tests_Assert(COMPARE_COLORS(cfogcol, &fogcol), "check fog color");
	Tests_Assert(COMPARE_COLORS(ccloudcol, &cloudcol), "check cloud color");
	Tests_Assert(COMPARE_COLORS(cambcol, &ambcol), "check ambient color");
	Tests_Assert(COMPARE_COLORS(cdiffcol, &diffcol), "check ambient color");

	Tests_NewTask("Check blocks placing");
	Tests_Assert(World_GetBlock(world, &p1) == BLOCK_BEDROCK, "check first block");
	Tests_Assert(World_GetBlock(world, &p2) == BLOCK_LOG, "check second block");
	Tests_Assert(World_GetBlockO(world, wsize - 1) == BLOCK_WATER_STILL, "check third block by offset");
	Tests_Assert(World_GetBlockO(world, wsize) == (BlockID)-1, "check block outside world");
	World_FreeBlockArray(world);
	World_Free(world);

	return true;
}
