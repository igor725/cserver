#include <core.h>
#include <world.h>

#include "script.h"
#include "lworld.h"
#include "lmath.h"

#define LUA_TWORLD "world"

World* luax_checkworld(lua_State* L, cs_int32 idx) {
	return luax_checkptr(L, idx, LUA_TWORLD);
}

static void setupworld(lua_State* L, void* obj) {
	World* world = (World*)obj;
	lua_getfield(L, LUA_REGISTRYINDEX, "cs_udata");

	lua_pushvalue(L, -2);
	lua_newtable(L);

	luax_newpfvec(L, &world->info->spawnVec);
	lua_setfield(L, -2, "sv");

	luax_newpang(L, &world->info->spawnAng);
	lua_setfield(L, -2, "sa");

	luax_newpsvec(L, &world->info->dimensions);
	lua_setfield(L, -2, "dv");

	lua_settable(L, -3);
	lua_pop(L, 1);
}

void luax_pushworld(lua_State* L, World* world) {
	luax_pushptr(L, world, LUA_TWORLD, setupworld);
}

LUA_SFUNC(mworld_save) {
	World* world = luax_checkworld(L, 1);
	cs_bool unload = (cs_bool)lua_toboolean(L, 2);
	lua_pushboolean(L, World_Save(world, unload));
	return 1;
}

LUA_SFUNC(mworld_load) {
	World* world = luax_checkworld(L, 1);
	lua_pushboolean(L, World_Load(world));
	return 1;
}

LUA_SFUNC(mworld_unload) {
	World* world = luax_checkworld(L, 1);
	World_Unload(world);
	lua_pushboolean(L, world->data == NULL);
	return 1;
}

LUA_SFUNC(mworld_getname) {
	World* world = luax_checkworld(L, 1);

	lua_pushstring(L, world->name);
	return 1;
}

LUA_SFUNC(mworld_getspawn) {
	luax_pushrgtableof(L, 1);
	lua_getfield(L, -1, "sv");
	lua_getfield(L, -2, "sa");
	return 2;
}

LUA_SFUNC(mworld_getdim) {
	luax_pushrgtableof(L, 1);
	lua_getfield(L, -1, "dv");
	return 1;
}

LUA_SFUNC(mworld_getblock) {
	World* world = luax_checkworld(L, 1);
	SVec* pos = luax_checksvec(L, 2);
	lua_pushinteger(L, World_GetBlock(world, pos));
	return 1;
}

LUA_SFUNC(mworld_getenvprop) {
	World* world = luax_checkworld(L, 1);
	cs_uint8 prop = (cs_uint8)luaL_checkinteger(L, 2);
	lua_pushinteger(L, World_GetProperty(world, prop));
	return 1;
}

LUA_SFUNC(mworld_getweather) {
	World* world = luax_checkworld(L, 1);
	lua_pushinteger(L, World_GetWeather(world));
	return 1;
}

LUA_SFUNC(mworld_setblock) {
	World* world = luax_checkworld(L, 1);
	SVec* pos = luax_checksvec(L, 2);
	BlockID id = (BlockID)luaL_checkinteger(L, 3);
	lua_pushboolean(L, World_SetBlock(world, pos, id));
	return 1;
}

LUA_SFUNC(mworld_setenvprop) {
	World* world = luax_checkworld(L, 1);
	cs_uint8 prop = (cs_uint8)luaL_checkinteger(L, 2);
	cs_int32 value = (cs_int32)luaL_checkinteger(L, 3);
	lua_pushboolean(L, World_SetProperty(world, prop, value));
	return 1;
}

LUA_SFUNC(mworld_setweather) {
	World* world = luax_checkworld(L, 1);
	Weather wt = (Weather)luaL_checkinteger(L, 2);
	lua_pushboolean(L, World_SetWeather(world, wt));
	return 1;
}

LUA_SFUNC(mworld_update) {
	World* world = luax_checkworld(L, 1);
	World_UpdateClients(world);
	return 0;
}

static const luaL_Reg world_methods[] = {
	{"save", mworld_save},
	{"load", mworld_load},
	{"unload", mworld_unload},

	{"getname", mworld_getname},
	{"getspawn", mworld_getspawn},
	{"getdim", mworld_getdim},
	{"getblock", mworld_getblock},
	{"getenvprop", mworld_getenvprop},
	{"getweather", mworld_getweather},

	{"setblock", mworld_setblock},
	{"setenvprop", mworld_setenvprop},
	{"setweather", mworld_setweather},

	{"update", mworld_update},

	{NULL, NULL}
};

LUA_SFUNC(fworld_getbyname) {
	const char* name = luaL_checkstring(L, 1);
	World* world = World_GetByName(name);

	if(world)
		luax_pushworld(L, world);
	else
		lua_pushnil(L);

	return 1;
}

LUA_SFUNC(fworld_getbyid) {
	WorldID wid = (WorldID)luaL_checkinteger(L, 1);
	World* world = World_GetByID(wid);

	if(world)
		luax_pushworld(L, world);
	else
		lua_pushnil(L);

	return 1;
}

LUA_SFUNC(fworld_create) {
	const char* name = luaL_checkstring(L, 1);
	const SVec* dims = luax_checksvec(L, 2);
	World* world = World_Create(name);
	World_SetDimensions(world, dims);
	World_AllocBlockArray(world);

	if(World_Add(world))
		luax_pushworld(L, world);
	else {
		lua_pushnil(L);
		World_Free(world);
	}

	return 1;
};

static const luaL_Reg world_funcs[] = {
	{"getbyname", fworld_getbyname},
	{"getbyid", fworld_getbyid},
	{"create", fworld_create},

	{NULL, NULL}
};

LUA_FUNC(luaopen_world) {
	luaL_newmetatable(L, LUA_TWORLD);
	lua_newtable(L);
	luaL_setfuncs(L, world_methods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lptr_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newlib(L, world_funcs);
	return 1;
}
