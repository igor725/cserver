#include <core.h>
#include <world.h>
#include <client.h>

#include "script.h"
#include "lclient.h"
#include "lworld.h"

#define LUA_TCLIENT "client"

Client luax_checkclient(lua_State* L, cs_int32 idx) {
	return luax_checkptr(L, idx, LUA_TCLIENT);
}

void luax_pushclient(lua_State* L, Client client) {
	luax_pushmyptr(L, client, LUA_TCLIENT, NULL);
}

LUA_SFUNC(mclient_getid) {
	Client client = luax_checkclient(L, 1);

	lua_pushinteger(L, client->id);
	return 1;
}

LUA_SFUNC(mclient_getname) {
	Client client = luax_checkclient(L, 1);

	lua_pushstring(L, Client_GetName(client));
	return 1;
}

LUA_SFUNC(mclient_getappname) {
	Client client = luax_checkclient(L, 1);

	lua_pushstring(L, Client_GetAppName(client));
	return 1;
}

LUA_SFUNC(mclient_getworld) {
	Client client = luax_checkclient(L, 1);

	luax_pushworld(L, Client_GetWorld(client));
	return 1;
}

LUA_SFUNC(mclient_getgroupid) {
	Client client = luax_checkclient(L, 1);

	lua_pushinteger(L, Client_GetGroupID(client));
	return 1;
}

LUA_SFUNC(mclient_getmodel) {
	Client client = luax_checkclient(L, 1);

	lua_pushinteger(L, Client_GetModel(client));
	return 1;
}

LUA_SFUNC(mclient_getheldblock) {
	Client client = luax_checkclient(L, 1);

	lua_pushinteger(L, Client_GetHeldBlock(client));
	return 1;
}

LUA_SFUNC(mclient_getextver) {
	Client client = luax_checkclient(L, 1);
	cs_uint32 crc32 = (cs_uint32)luaL_checkinteger(L, 2);

	lua_pushinteger(L, Client_GetExtVer(client, crc32));
	return 1;
}

LUA_SFUNC(mclient_setenvprop) {
	Client client = luax_checkclient(L, 1);
	cs_uint8 prop = (cs_uint8)luaL_checkinteger(L, 2);
	cs_uint32 val = (cs_uint32)luaL_checkinteger(L, 3);

	lua_pushboolean(L, Client_SetEnvProperty(client, prop, val));
	return 1;
}

LUA_SFUNC(mclient_setweather) {
	Client client = luax_checkclient(L, 1);
	Weather wt = (Weather)luaL_checkinteger(L, 2);

	lua_pushboolean(L, Client_SetWeather(client, wt));
	return 1;
}

LUA_SFUNC(mclient_setinvorder) {
	Client client = luax_checkclient(L, 1);
	Order order = (Order)luaL_checkinteger(L, 2);
	BlockID block = (BlockID)luaL_checkinteger(L, 3);

	lua_pushboolean(L, Client_SetInvOrder(client, order, block));
	return 1;
}

LUA_SFUNC(mclient_setheldblock) {
	Client client = luax_checkclient(L, 1);
	BlockID block = (BlockID)luaL_checkinteger(L, 2);
	cs_bool lock = (cs_bool)lua_toboolean(L, 3);

	lua_pushboolean(L, Client_SetHeld(client, block, lock));
	return 1;
}

LUA_SFUNC(mclient_sethotkey) {
	Client client = luax_checkclient(L, 1);
	const char* action = luaL_checkstring(L, 2);
	cs_int32 keycode = (cs_int32)luaL_checkinteger(L, 3);
	cs_int8 keymod = (cs_int8)luaL_checkinteger(L, 4);

	lua_pushboolean(L, Client_SetHotkey(client, action, keycode, keymod));
	return 1;
}

LUA_SFUNC(mclient_sethotbar) {
	Client client = luax_checkclient(L, 1);
	Order pos = (Order)luaL_checkinteger(L, 2);
	BlockID block = (BlockID)luaL_checkinteger(L, 3);

	lua_pushboolean(L, Client_SetHotbar(client, pos, block));
	return 1;
}

LUA_SFUNC(mclient_setblockperm) {
	Client client = luax_checkclient(L, 1);
	BlockID block = (BlockID)luaL_checkinteger(L, 2);
	cs_bool canPlace = (cs_bool)lua_toboolean(L, 3);
	cs_bool canDestroy = (cs_bool)lua_toboolean(L, 4);

	lua_pushboolean(L, Client_SetBlockPerm(client, block, canPlace, canDestroy));
	return 1;
}

LUA_SFUNC(mclient_setmodel) {
	Client client = luax_checkclient(L, 1);
	cs_int16 model = (cs_int16)luaL_checkinteger(L, 2);

	lua_pushboolean(L, Client_SetModel(client, model));
	return 1;
}

LUA_SFUNC(mclient_setskin) {
	Client client = luax_checkclient(L, 1);
	const char* skin = luaL_checkstring(L, 2);

	lua_pushboolean(L, Client_SetSkin(client, skin));
	return 1;
}

LUA_SFUNC(mclient_setgroup) {
	Client client = luax_checkclient(L, 1);
	cs_int16 group = (cs_int16)luaL_checkinteger(L, 2);

	lua_pushboolean(L, Client_SetGroup(client, group));
	return 1;
}

LUA_SFUNC(mclient_spawn) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_Spawn(client));
	return 1;
}

LUA_SFUNC(mclient_update) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_Update(client));
	return 1;
}

LUA_SFUNC(mclient_despawn) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_Despawn(client));
	return 1;
}

LUA_SFUNC(mclient_kick) {
	Client client = luax_checkclient(L, 1);
	const char* reason = luaL_checkstring(L, 2);

	Client_Kick(client, reason);
	return 0;
}

LUA_SFUNC(mclient_isingame) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_IsInGame(client));
	return 1;
}

LUA_SFUNC(mclient_iscpe) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, client->cpeData != NULL);
	return 1;
};

LUA_SFUNC(mclient_isop) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_IsOP(client));
	return 1;
}

LUA_SFUNC(mclient_sendchat) {
	Client client = luax_checkclient(L, 1);
	MessageType type = (MessageType)luaL_checkinteger(L, 2);
	const char* name = luaL_checkstring(L, 2);

	Client_Chat(client, type, name);
	return 0;
}

static const luaL_Reg client_methods[] = {
	{"getid", mclient_getid},
	{"getname", mclient_getname},
	{"getappname", mclient_getappname},
	{"getworld", mclient_getworld},
	{"getgroupid", mclient_getgroupid},
	{"getmodel", mclient_getmodel},
	{"getheldblock", mclient_getheldblock},
	// {"getextver", mclient_getextver},

	{"setenvprop", mclient_setenvprop},
	{"setweather", mclient_setweather},
	{"setinvorder", mclient_setinvorder},
	{"setheldblock", mclient_setheldblock},
	{"sethotkey", mclient_sethotkey},
	{"sethotbar", mclient_sethotbar},
	{"setblockperm", mclient_setblockperm},
	{"setmodel", mclient_setmodel},
	{"setskin", mclient_setskin},
	{"setgroup", mclient_setgroup},

	{"update", mclient_update},
	{"despawn", mclient_despawn},
	{"spawn", mclient_spawn},
	{"kick", mclient_kick},

	{"isingame", mclient_isingame},
	{"iscpe", mclient_iscpe},
	{"isop", mclient_isop},

	{"sendchat", mclient_sendchat},

	{NULL, NULL}
};

LUA_SFUNC(fclient_getbyid) {
	ClientID id = (ClientID)luaL_checkinteger(L, 1);
	Client client = Client_GetByID(id);

	if(client)
		luax_pushclient(L, client);
	else
		lua_pushnil(L);
	return 1;
}

LUA_SFUNC(fclient_getbyname) {
	const char* name = luaL_checkstring(L, 1);
	Client client = Client_GetByName(name);

	if(client)
		luax_pushclient(L, client);
	else
		lua_pushnil(L);
	return 1;
}

static const luaL_Reg client_funcs[] = {
	{"getbyid", fclient_getbyid},
	{"getbyname", fclient_getbyname},

	{NULL, NULL}
};

LUA_FUNC(luaopen_client) {
	luaL_newmetatable(L, LUA_TCLIENT);
	lua_newtable(L);
	luaL_setfuncs(L, client_methods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lptr_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newlib(L, client_funcs);
	return 1;
}
