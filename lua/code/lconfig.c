#include <core.h>
#include <config.h>
#include <server.h>

#include "script.h"

#define LUA_TSTORE "cfgStore"
#define LUA_TENTRY "cfgEntry"

CStore* luax_checkstore(lua_State* L, cs_int32 idx) {
	return luax_checkptr(L, idx, LUA_TSTORE);
}

void luax_pushstore(lua_State* L, CStore* store) {
	luax_pushptr(L, store, LUA_TSTORE, NULL);
}

CEntry* luax_checkentry(lua_State* L, cs_int32 idx) {
	return luax_checkptr(L, idx, LUA_TENTRY);
}

void luax_pushentry(lua_State* L, CEntry* entry) {
	luax_pushptr(L, entry, LUA_TENTRY, NULL);
}

LUA_SFUNC(mentry_set) {
	CEntry* ent = luax_checkentry(L, 1);
	luaL_checktype(L, 3, LUA_TBOOLEAN);
	cs_bool isDefault = (cs_bool)lua_toboolean(L, 3);
	cs_int32 i; cs_int16 h; cs_int8 b;
	const char* str;

	switch(ent->type) {
		case CFG_STR:
			str = luaL_checkstring(L, 2);
			if(isDefault)
				Config_SetDefaultStr(ent, str);
			else
				Config_SetStr(ent, str);
			break;
		case CFG_INT32:
			i = (cs_int32)luaL_checkinteger(L, 2);
			Config_SetInt32(ent, i);
			break;
		case CFG_INT16:
			h = (cs_int16)luaL_checkinteger(L, 2);
			if(isDefault)
				Config_SetDefaultInt16(ent, h);
			else
				Config_SetInt16(ent, h);
			break;
		case CFG_INT8:
			b = (cs_int8)luaL_checkinteger(L, 2);
			if(isDefault)
				Config_SetDefaultInt8(ent, b);
			else
				Config_SetInt8(ent, b);
			break;
		case CFG_BOOL:
			luaL_checktype(L, 2, LUA_TBOOLEAN);
			b = (cs_bool)lua_toboolean(L, 2);
			if(isDefault)
				Config_SetDefaultBool(ent, b);
			else
				Config_SetBool(ent, b);
			break;
		default:
			luaL_error(L, "Invalid entry type.");
	}

	return 0;
}

LUA_SFUNC(mentry_get) {
	CEntry* ent = luax_checkentry(L, 1);
	switch(ent->type) {
		case CFG_STR:
			lua_pushstring(L, Config_GetStr(ent));
			break;
		case CFG_INT32:
			lua_pushinteger(L, Config_GetInt32(ent));
			break;
		case CFG_INT16:
			lua_pushinteger(L, Config_GetInt16(ent));
			break;
		case CFG_INT8:
			lua_pushinteger(L, Config_GetInt8(ent));
			break;
		case CFG_BOOL:
			lua_pushboolean(L, Config_GetBool(ent));
			break;
		default:
			luaL_error(L, "Invalid entry type.");
	}
	return 1;
}

static const luaL_Reg entry_methods[] = {
	{"set", mentry_set},
	{"get", mentry_get},

	{NULL, NULL}
};

LUA_SFUNC(mstore_entry) {
	CStore* store = luax_checkstore(L, 1);
	const char* key = luaL_checkstring(L, 2);
	CEntry* ent = Config_GetEntry(store, key);
	if(!ent) {
		const char* stype = luaL_checkstring(L, 3);
		cs_int32 ntype = Config_TypeNameToInt(stype);
		ent = Config_NewEntry(store, key, ntype);
	}
	luax_pushentry(L, ent);
	return 1;
}

LUA_SFUNC(mstore_load) {
	CStore* store = luax_checkstore(L, 1);
	lua_pushboolean(L, Config_Load(store));
	return 1;
}

LUA_SFUNC(mstore_save) {
	CStore* store = luax_checkstore(L, 1);
	lua_pushboolean(L, Config_Save(store));
	return 1;
}

LUA_SFUNC(mstore_destroy) {
	CStore* store = luax_checkstore(L, 1);
	lptr_gc(L);
	Config_DestroyStore(store);
	return 0;
}

static const luaL_Reg store_methods[] = {
	{"entry", mstore_entry},

	{"load", mstore_load},
	{"save", mstore_save},

	{"destroy", mstore_destroy},

	{NULL, NULL}
};

LUA_SFUNC(fcfg_new) {
	const char* path = luaL_checkstring(L, 1);
	CStore* store = Config_NewStore(path);
	luax_pushstore(L, store);
	return 1;
}

static const luaL_Reg cfg_funcs[] = {
  {"new", fcfg_new},

  {NULL, NULL}
};

LUA_FUNC(luaopen_config) {
	luaL_newmetatable(L, LUA_TSTORE);
	lua_newtable(L);
	luaL_setfuncs(L, store_methods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lptr_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newmetatable(L, LUA_TENTRY);
	lua_newtable(L);
	luaL_setfuncs(L, entry_methods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lptr_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newlib(L, cfg_funcs);

	luax_pushstore(L, Server_Config);
	lua_setfield(L, -2, "server");

	return 1;
}
