#include <core.h>
#include <world.h>

#include "script.h"
#include "lworld.h"

#define LUA_TWORLD "world"

World luax_checkworld(lua_State* L, cs_int32 idx) {
	return luax_checkptr(L, idx, LUA_TWORLD);
}

void luax_pushworld(lua_State* L, World world) {
	luax_pushmyptr(L, world, LUA_TWORLD);
}

LUA_SFUNC(lworld_getbyname) {
	const char* name = luaL_checkstring(L, 1);
	World world = World_GetByName(name);

	if(world)
		luax_pushworld(L, world);
	else
		lua_pushnil(L);

	return 1;
}

LUA_SFUNC(lworld_getname) {
	World world = luax_checkworld(L, 1);

	lua_pushstring(L, world->name);
	return 1;
}

static const luaL_Reg worldmethods[] = {
	{"getname", lworld_getname},

	{NULL, NULL}
};

static const luaL_Reg worldfuncs[] = {
	{"getbyname", lworld_getbyname},

	{NULL, NULL}
};

LUA_FUNC(luaopen_world) {
	luaL_newmetatable(L, LUA_TWORLD);
	lua_newtable(L);
	luaL_setfuncs(L, worldmethods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lptr_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newlib(L, worldfuncs);
	return 1;
}
