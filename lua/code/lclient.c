#include <core.h>
#include <client.h>

#include "script.h"
#include "lclient.h"
#include "lworld.h"

#define LUA_TCLIENT "client"

Client luax_checkclient(lua_State* L, cs_int32 idx) {
	return luax_checkmyobject(L, idx, LUA_TCLIENT);
}

void luax_pushclient(lua_State* L, Client client) {
	luax_pushmyobject(L, client, LUA_TCLIENT);
}

static cs_int32 lclient_getbyid(lua_State* L) {
	ClientID id = (ClientID)luaL_checkinteger(L, 1);
	Client client = Client_GetByID(id);

	if(client)
		luax_pushclient(L, client);
	else
		lua_pushnil(L);
	return 1;
}

static cs_int32 lclient_getbyname(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	Client client = Client_GetByName(name);

	if(client)
		luax_pushclient(L, client);
	else
		lua_pushnil(L);
	return 1;
}

static cs_int32 lclient_getid(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushinteger(L, client->id);
	return 1;
}

static cs_int32 lclient_getname(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushstring(L, Client_GetName(client));
	return 1;
}

static cs_int32 lclient_getappname(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushstring(L, Client_GetAppName(client));
	return 1;
}

static cs_int32 lclient_getgroupid(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushinteger(L, Client_GetGroupID(client));
	return 1;
}

static cs_int32 lclient_getmodel(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushinteger(L, Client_GetModel(client));
	return 1;
}

static cs_int32 lclient_getheldblock(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushinteger(L, Client_GetHeldBlock(client));
	return 1;
}

static cs_int32 lclient_getextver(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	cs_uint32 crc32 = (cs_uint32)luaL_checkinteger(L, 2);

	lua_pushinteger(L, Client_GetExtVer(client, crc32));
	return 1;
}

static cs_int32 lclient_setenvprop(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	cs_uint8 prop = (cs_uint8)luaL_checkinteger(L, 2);
	cs_uint32 val = (cs_uint32)luaL_checkinteger(L, 3);

	lua_pushboolean(L, Client_SetEnvProperty(client, prop, val));
	return 1;
}

static cs_int32 lcient_setweather(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	Weather wt = (Weather)luaL_checkinteger(L, 2);

	lua_pushboolean(L, Client_SetWeather(client, wt));
	return 1;
}

static cs_int32 lclient_setinvorder(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	Order order = (Order)luaL_checkinteger(L, 2);
	BlockID block = (BlockID)luaL_checkinteger(L, 3);

	lua_pushboolean(L, Client_SetInvOrder(client, order, block));
	return 1;
}

static cs_int32 lclient_setheldblock(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	BlockID block = (BlockID)luaL_checkinteger(L, 2);
	cs_bool lock = (cs_bool)lua_toboolean(L, 3);

	lua_pushboolean(L, Client_SetHeld(client, block, lock));
	return 1;
}

static cs_int32 lclient_sethotkey(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	const char* action = luaL_checkstring(L, 2);
	cs_int32 keycode = (cs_int32)luaL_checkinteger(L, 3);
	cs_int8 keymod = (cs_int8)luaL_checkinteger(L, 4);

	lua_pushboolean(L, Client_SetHotkey(client, action, keycode, keymod));
	return 1;
}

static cs_int32 lclient_sethotbar(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	Order pos = (Order)luaL_checkinteger(L, 2);
	BlockID block = (BlockID)luaL_checkinteger(L, 3);

	lua_pushboolean(L, Client_SetHotbar(client, pos, block));
	return 1;
}

static cs_int32 lclient_setblockperm(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	BlockID block = (BlockID)luaL_checkinteger(L, 2);
	cs_bool canPlace = (cs_bool)lua_toboolean(L, 3);
	cs_bool canDestroy = (cs_bool)lua_toboolean(L, 4);

	lua_pushboolean(L, Client_SetBlockPerm(client, block, canPlace, canDestroy));
	return 1;
}

static cs_int32 lclient_setmodel(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	cs_int16 model = (cs_int16)luaL_checkinteger(L, 2);

	lua_pushboolean(L, Client_SetModel(client, model));
	return 1;
}

static cs_int32 lclient_setskin(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	const char* skin = luaL_checkstring(L, 2);

	lua_pushboolean(L, Client_SetSkin(client, skin));
	return 1;
}

static cs_int32 lclient_setgroup(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	cs_int16 group = (cs_int16)luaL_checkinteger(L, 2);

	lua_pushboolean(L, Client_SetGroup(client, group));
	return 1;
}

static cs_int32 lclient_spawn(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_Spawn(client));
	return 1;
}

static cs_int32 lclient_update(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_Update(client));
	return 1;
}

static cs_int32 lclient_despawn(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_Despawn(client));
	return 1;
}

static cs_int32 lclient_kick(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	const char* reason = luaL_checkstring(L, 2);

	Client_Kick(client, reason);
	return 0;
}

static cs_int32 lclient_isingame(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_IsInGame(client));
	return 1;
}

static cs_int32 lclient_isop(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushboolean(L, Client_IsOP(client));
	return 1;
}

static cs_int32 lclient_sendchat(lua_State* L) {
	Client client = luax_checkclient(L, 1);
	MessageType type = (MessageType)luaL_checkinteger(L, 2);
	const char* name = luaL_checkstring(L, 2);

	Client_Chat(client, type, name);
	return 0;
}

static cs_int32 lclient_gc(lua_State* L) {
	*(void**)luaL_checkudata(L, 1, LUA_TCLIENT) = NULL;
	return 0;
}

static const luaL_Reg clientfuncs[] = {
	{"getbyid", lclient_getbyid},
	{"getbyname", lclient_getbyname},
	{NULL, NULL}
};

static const luaL_Reg clientmethods[] = {
	{"getid", lclient_getid},
	{"getname", lclient_getname},
	// {"getworld", lclient_getworld}, // TODO: Написать lua-модуль "world"
	{"getappname", lclient_getappname},
	{"getgroupid", lclient_getgroupid},
	{"getmodel", lclient_getmodel},
	{"getheldblock", lclient_getheldblock},
	// {"getextver", lclient_getextver},

	{"setenvprop", lclient_setenvprop},
	{"setweather", lcient_setweather},
	{"setinvorder", lclient_setinvorder},
	{"setheldblock", lclient_setheldblock},
	{"sethotkey", lclient_sethotkey},
	{"sethotbar", lclient_sethotbar},
	{"setblockperm", lclient_setblockperm},
	{"setmodel", lclient_setmodel},
	{"setskin", lclient_setskin},
	{"setgroup", lclient_setgroup},

	{"update", lclient_update},
	{"despawn", lclient_despawn},
	{"spawn", lclient_spawn},
	{"kick", lclient_kick},

	{"isingame", lclient_isingame},
	{"isop", lclient_isop},

	{"sendchat", lclient_sendchat},

	{NULL, NULL}
};

static cs_int32 lclient_tostring(lua_State* L) {
	Client client = luax_checkclient(L, 1);

	lua_pushstring(L, Client_GetName(client));
	return 1;
}

cs_int32 luaopen_client(lua_State* L) {
	// Client metatable
	luaL_newmetatable(L, LUA_TCLIENT);
	lua_newtable(L);
	luaL_setfuncs(L, clientmethods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lclient_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newlib(L, clientfuncs);
	return 1;
}
