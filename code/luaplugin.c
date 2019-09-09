#ifdef LUA_ENABLED
#include "core.h"
#include "config.h"
#include "luaplugin.h"
#include "command.h"
#include "packets.h"
#include "event.h"

static void* testudata(lua_State* L, int index, const char* tname) {
	void* ptr = lua_touserdata(L, index);
	if(ptr) {
		if(lua_getmetatable(L, index)) {
			luaL_getmetatable(L, tname);
			if(!lua_rawequal(L, -1, -2))
				ptr = NULL;
			lua_pop(L, 2);
			return ptr;
		}
	}
	return NULL;
}

static void* checkudata(lua_State* L, int index, const char* tname) {
	void* ptr = testudata(L, index, tname);
	if(!ptr) luaL_typerror(L, index, tname);
	return ptr;
}

static PLUGIN* FindByState(lua_State* L) {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(ptr->state == L) {
			return ptr;
		}
		ptr = ptr->next;
	}

	return (PLUGIN*)NULL;
}

static int LuaError(lua_State* L) {
	PLUGIN* plugin = FindByState(L);
	if(plugin)
		Log_Error("In plugin \"%s\": %s", plugin->name, lua_tostring(L, -1));
	else
		Log_Error(lua_tostring(L, -1));
	return 0;
}

/*
	Push/pull/check functions
*/

#define LUA_TWORLD "classicWorld"
#define LUA_TCFGSTORE "cfgStore"
#define LUA_TCLIENT "client"

static WORLD* toWorld(lua_State* L, int index) {
	WORLD* world = lua_touserdata(L, index);
	if(!world) luaL_typerror(L, index, LUA_TWORLD);
	return world;
}

static WORLD* checkWorld(lua_State* L, int index) {
	return checkudata(L, index, LUA_TWORLD);
}

static void pushWorld(lua_State* L, WORLD* world) {
	lua_pushlightuserdata(L, world);
	luaL_getmetatable(L, LUA_TWORLD);
	lua_setmetatable(L, -2);
}

static CLIENT* toClient(lua_State* L, int index) {
	CLIENT* client = lua_touserdata(L, index);
	if(!client) luaL_typerror(L, index, LUA_TCLIENT);
	return client;
}

static CLIENT* checkClient(lua_State* L, int index) {
	return checkudata(L, index, LUA_TCLIENT);
}

static void pushClient(lua_State* L, CLIENT* client) {
	lua_pushlightuserdata(L, client);
	luaL_getmetatable(L, LUA_TCLIENT);
	lua_setmetatable(L, -2);
}

static CFGSTORE* toStore(lua_State* L, int index) {
	CFGSTORE* store = lua_touserdata(L, index);
	if(!store) luaL_typerror(L, index, LUA_TCFGSTORE);
	return store;
}

static CFGSTORE* checkStore(lua_State* L, int index) {
	CFGSTORE* store;
	luaL_checktype(L, index, LUA_TUSERDATA);
	store = luaL_checkudata(L, index, LUA_TCFGSTORE);
	if(!store) luaL_typerror(L, index, LUA_TCFGSTORE);
	return store;
}

static CFGSTORE* pushStore(lua_State* L) {
	CFGSTORE* store = lua_newuserdata(L, sizeof(CFGSTORE));
	luaL_getmetatable(L, LUA_TCFGSTORE);
	lua_setmetatable(L, -2);
	return store;
}

#define LuaCallback_Start(plugin, func) \
lua_pushcfunction(plugin->state, LuaError); \
lua_getglobal(plugin->state, func); \
if(lua_isfunction(plugin->state, -1)) { \

#define LuaCallback_Call(plugin, argc, out) \
lua_pcall(plugin->state, argc, out, -2 - argc) \

#define LuaCallback_End(plugin) \
} else { \
	lua_pop(plugin->state, 2); \
} \

#define CallLuaVoid(plugin, func) \
LuaCallback_Start(plugin, func); \
LuaCallback_Call(plugin, 0, 0); \
LuaCallback_End(plugin); \

#define GetLuaPlugin \
PLUGIN* plugin = FindByState(L); \
if(!plugin) \
	return 0; \

#define VarStrConcat \
int argc = lua_gettop(L); \
char buf[4096] = {0}; \
size_t bufpos = 0; \
if(argc > 0) { \
	for(int i = 1; i <= argc; i++) { \
		lua_getglobal(L, "tostring"); \
		lua_pushvalue(L, i); \
		lua_call(L, 1, 1); \
		const char* str = lua_tostring(L, -1); \
		lua_pop(L, 1); \
		bufpos += String_Copy(buf + bufpos, 4096 - bufpos, str); \
		if(bufpos < 4094) { \
			buf[bufpos] = 32; \
			++bufpos; \
		} \
	} \
} \

/*
	Lua log library
*/

static int llog_info(lua_State* L) {
	GetLuaPlugin;
	VarStrConcat;
	Log_Info("%s: %s", plugin->name, buf);
	return 0;
}

static int llog_error(lua_State* L) {
	GetLuaPlugin;
	VarStrConcat;
	Log_Error("%s: %s", plugin->name, buf);
	return 0;
}

static int llog_warn(lua_State* L) {
	GetLuaPlugin;
	VarStrConcat;
	Log_Error("%s: %s", plugin->name, buf);
	return 0;
}

static const luaL_Reg loglib[] = {
	{"info", llog_info},
	{"error", llog_error},
	{"warn", llog_warn},
	{NULL, NULL}
};

static int luaopen_log(lua_State* L) {
	luaL_register(L, lua_tostring(L, -1), loglib);
	return 1;
}

/*
	Lua world library
*/

static int lworld_get(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	WORLD* world = World_FindByName(name);

	if(!world)
		lua_pushnil(L);
	else
		pushWorld(L, world);

	return 1;
}

static int lworld_setblock(lua_State* L) {
	WORLD* world = toWorld(L, 1);
	ushort x = (ushort)luaL_checkint(L, 2);
	ushort y = (ushort)luaL_checkint(L, 3);
	ushort z = (ushort)luaL_checkint(L, 4);
	BlockID id = (BlockID)luaL_checkint(L, 5);
	bool update = (bool)lua_toboolean(L, 6);

	World_SetBlock(world, x, y, z, id);
	if(update)
		Client_UpdateBlock(NULL, world, x, y, z);

	return 0;
}

static int lworld_getblock(lua_State* L) {
	WORLD* world = toWorld(L, 1);
	ushort x = (ushort)luaL_checkint(L, 2);
	ushort y = (ushort)luaL_checkint(L, 3);
	ushort z = (ushort)luaL_checkint(L, 4);

	lua_pushinteger(L, World_GetBlock(world, x, y, z));
	return 1;
}

static const luaL_Reg world_methods[] = {
	{"get", lworld_get},
	{"setblock", lworld_setblock},
	{"getblock", lworld_getblock},
	{NULL, NULL}
};

static int lworld_tostring(lua_State* L) {
	WORLD* world = toWorld(L, 1);
	lua_pushstring(L, world->name);
	return 1;
}

static const luaL_Reg world_meta[] = {
	{"__tostring", lworld_tostring},
	{NULL, NULL}
};

static int luaopen_world(lua_State* L) {
	luaL_openlib(L, lua_tostring(L, -1), world_methods, 0);

	luaL_newmetatable(L, LUA_TWORLD);

	luaL_openlib(L, 0, world_meta, 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushstring(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}

/*
	Lua client library
*/

static int lclient_get(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	CLIENT* client = Client_FindByName(name);

	if(!client)
		lua_pushnil(L);
	else
		pushClient(L, client);

	return 1;
}

static int lclient_isop(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	lua_pushboolean(L, Client_GetType(client));
	return 1;
}

static int lclient_issuportext(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	const char* ext = luaL_checkstring(L, 2);
	lua_pushboolean(L, Client_IsSupportExt(client, ext));
	return 1;
}

static int lclient_getname(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	if(!client->playerData)
		luaL_error(L, "client->playerData == NULL");
	lua_pushstring(L, client->playerData->name);
	return 1;
}

static int lclient_getappname(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	lua_pushstring(L, Client_GetAppName(client));
	return 1;
}

static int lclient_setop(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	bool isOP = lua_toboolean(L, 2);
	lua_pushboolean(L, Client_SetType(client, isOP));
	return 0;
}

static int lclient_sethotbar(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	Order pos = (Order)luaL_checkint(L, 2);
	BlockID block = (BlockID)luaL_checkint(L, 3);

	lua_pushboolean(L, Client_SetHotbar(client, pos, block));
	return 1;
}

static int lclient_changeworld(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	WORLD* world = checkWorld(L, 2);
	lua_pushboolean(L, Client_ChangeWorld(client, world));
	return 1;
}

static int lclient_sendmessage(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	MessageType t = (MessageType)luaL_checkint(L, 2);
	const char* msg = luaL_checkstring(L, 3);
	Packet_WriteChat(client, t, msg);
	return 0;
}

static int lclient_disconnect(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	Client_Disconnect(client);
	return 0;
}

static int lclient_despawn(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	lua_pushboolean(L, Client_Despawn(client));
	return 1;
}

static int lclient_spawn(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	lua_pushboolean(L, Client_Spawn(client));
	return 1;
}

static int lclient_kick(lua_State* L) {
	CLIENT* client = checkClient(L, 1);
	const char* reason = luaL_checkstring(L, 2);
	Client_Kick(client, reason);
	return 0;
}

static const luaL_Reg client_methods[] = {
	{"get", lclient_get},
	{"getname", lclient_getname},
	{"getappname", lclient_getappname},

	{"isop", lclient_isop},
	{"issupportext", lclient_issuportext},

	{"setop", lclient_setop},
	{"sethotbar", lclient_sethotbar},

	{"changeworld", lclient_changeworld},
	{"sendmessage", lclient_sendmessage},
	{"disconnect", lclient_disconnect},
	{"despawn", lclient_despawn},
	{"spawn", lclient_spawn},
	{"kick", lclient_kick},
	{NULL, NULL}
};

static int lclient_tostring(lua_State* L) {
	CLIENT* client = toClient(L, 1);
	lua_pushstring(L, Client_GetName(client));
	return 1;
}

static const luaL_Reg client_meta[] = {
	{"__tostring", lclient_tostring},
	{NULL, NULL}
};

static int luaopen_client(lua_State* L) {
	luaL_openlib(L, lua_tostring(L, -1), client_methods, 0);

	luaL_newmetatable(L, LUA_TCLIENT);

	luaL_openlib(L, 0, client_meta, 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushstring(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}

/*
	Lua config library
*/

static int lconfig_newStore(lua_State* L) {
	const char* filename = luaL_checkstring(L, 1);
	if(String_CaselessCompare(filename, MAINCFG)) {
		luaL_error(L, "Can't create cfgStore with reserved filename");
	}

	CFGSTORE* store = pushStore(L);
	store->path = String_AllocCopy(filename);
	store->firstCfgEntry = NULL;
	store->lastCfgEntry = NULL;
	store->modified = false;
	return 1;
}

static int lconfig_purge(lua_State* L) {
	CFGSTORE* store = checkStore(L, 1);
	Config_EmptyStore(store);
	return 0;
}

static int lconfig_load(lua_State* L) {
	CFGSTORE* store = checkStore(L, 1);

	if(!Config_Load(store)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, Error_GetString());
	} else {
		lua_pushboolean(L, true);
		lua_pushnil(L);
	}

	return 2;
}

static int lconfig_save(lua_State* L) {
	CFGSTORE* store = checkStore(L, 1);

	if(!Config_Save(store)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, Error_GetString());
	} else {
		lua_pushboolean(L, true);
		lua_pushnil(L);
	}

	return 2;
}

static int lconfig_set(lua_State* L) {
	CFGSTORE* store = checkStore(L, 1);
	const char* key = luaL_checkstring(L, 2);
	const char* err = NULL;
	bool succ = true;

	switch(lua_type(L, 3)) {
		case LUA_TBOOLEAN:
			Config_SetBool(store, key, lua_toboolean(L, 3));
			break;
		case LUA_TSTRING:
			Config_SetStr(store, key, luaL_checkstring(L, 3));
			break;
		case LUA_TNUMBER:
			Config_SetInt(store, key, (int)luaL_checkint(L, 3));
			break;
		default:
			err = "Bad argument #2 (number/boolean/string expected)";
			succ = false;
			break;
	}

	lua_pushboolean(L, succ);
	if(err)
		lua_pushstring(L, err);
	else
		lua_pushnil(L);
	return 2;
}

static int lconfig_get(lua_State* L) {
	CFGSTORE* store = checkStore(L, 1);
	const char* key = luaL_checkstring(L, 2);
	CFGENTRY* ent = Config_GetEntry(store, key);

	if(!ent) {
		lua_pushnil(L);
		return 1;
	}

	switch (ent->type) {
		case CFG_BOOL:
			lua_pushboolean(L, ent->value.vbool);
			break;
		case CFG_INT:
			lua_pushinteger(L, ent->value.vint);
			break;
		case CFG_STR:
			lua_pushstring(L, ent->value.vchar);
			break;
		default:
			luaL_error(L, "Internal error, unknown cfgentry type");
			break;
	}

	return 1;
}

static int lconfig_type(lua_State* L) {
	CFGSTORE* store = checkStore(L, 1);
	const char* key = luaL_checkstring(L, 2);
	CFGENTRY* ent = Config_GetEntry(store, key);

	if(!ent) {
		lua_pushstring(L, "nil");
		return 1;
	}
	const char* type;

	switch (ent->type) {
		case CFG_BOOL:
			type = CFG_BOOL_NAME;
			break;
		case CFG_INT:
			type = CFG_INT_NAME;
			break;
		case CFG_STR:
			type = CFG_STR_NAME;
			break;
		default:
			type = CFG_UNK_NAME;
			luaL_error(L, "Internal error: unknown cfgentry type");
			break;
	}

	lua_pushstring(L, type);
	return 1;
}

static int lconfig_istype(lua_State* L) {
	CFGSTORE* store = checkStore(L, 1);
	const char* key = luaL_checkstring(L, 2);
	const char* type = luaL_checkstring(L, 3);
	CFGENTRY* ent = Config_GetEntry(store, key);
	bool valid = false;

	switch (ent->type) {
		case CFG_BOOL:
			valid = String_Compare(type, CFG_BOOL_NAME);
			break;
		case CFG_INT:
			valid = String_Compare(type, CFG_INT_NAME);
			break;
		case CFG_STR:
			valid = String_Compare(type, CFG_STR_NAME);
			break;
	}

	lua_pushboolean(L, valid);
	return 1;
}

static const luaL_Reg cfg_methods[] = {
	{"newstore", lconfig_newStore},
	{"purge", lconfig_purge},
	{"save", lconfig_save},
	{"load", lconfig_load},
	{"get", lconfig_get},
	{"set", lconfig_set},
	{"type", lconfig_type},
	{"istype", lconfig_istype},
	{NULL, NULL},
};

static int lconfig_gc(lua_State* L) {
	CFGSTORE* store = toStore(L, 1);
	Config_EmptyStore(store);
	return 0;
}

static int lconfig_tostring(lua_State* L) {
	lua_pushstring(L, LUA_TCFGSTORE);
	return 1;
}

static const luaL_Reg cfg_meta[] = {
	{"__gc", lconfig_gc},
	{"__tostring", lconfig_tostring},
	{NULL, NULL}
};

static int luaopen_config(lua_State* L) {
	luaL_openlib(L, lua_tostring(L, -1), cfg_methods, 0);

	luaL_newmetatable(L, LUA_TCFGSTORE);

	luaL_openlib(L, 0, cfg_meta, 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushstring(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}

/*
	Lua packets library
*/

// static const luaL_Reg packetslib[] = {
// 	{NULL, NULL}
// };
//
// static int luaopen_packets(lua_State* L) {
// 	luaL_register(L, lua_tostring(L, -1), packetslib);
// 	return 1;
// }

static const luaL_Reg LuaPlugin_Libs[] = {
	{"", luaopen_base},
	{"log", luaopen_log},
	{"client", luaopen_client},
	{"config", luaopen_config},
	// {"packets", luaopen_packets},
	{"world", luaopen_world},
	{LUA_TABLIBNAME, luaopen_table},
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{LUA_DBLIBNAME, luaopen_debug},
	{NULL, NULL}
};

/*
	Lua global functions
*/

static int lunload(lua_State* L) {
	PLUGIN* pl = FindByState(L);
	if(pl)
		pl->loaded = false;
	return 0;
}

static const luaL_Reg LuaPlugin_Globals[] = {
	{"_unload", lunload},
	{NULL, NULL}
};

void LuaPlugin_LoadLibs(lua_State* L) {
	const luaL_Reg* reg;
	for(reg = LuaPlugin_Libs; reg->func; reg++) {
		lua_pushcfunction(L, reg->func);
		lua_pushstring(L, reg->name);
		lua_call(L, 1, 0);
	}

	for(reg = LuaPlugin_Globals; reg->func; reg++) {
		lua_pushcfunction(L, reg->func);
		lua_setglobal(L, reg->name);
	}

	lua_getglobal(L, "log");
	lua_getfield(L, -1, "info");
	lua_setglobal(L, "print");
	lua_pop(L, 1);
}

static bool LuaPlugin_CanLoad(const char* name) {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(String_CaselessCompare(ptr->name, name)) {
			return false;
		}
		ptr = ptr->next;
	}

	return true;
}

#define checkCompatibility(ver) (LPLUGIN_API_NUM / 100 == ver / 100)

bool LuaPlugin_Load(const char* name) {
	if(!LuaPlugin_CanLoad(name)) {
		Log_Error("Plugin %q already loaded.", name);
		return false;
	}

	char path[MAX_PATH];
	String_FormatBuf(path, MAX_PATH, "plugins/%s", name);

	lua_State* L = luaL_newstate();
	LuaPlugin_LoadLibs(L);
	PLUGIN* tmp = Memory_Alloc(1, sizeof(PLUGIN));
	tmp->name = String_AllocCopy(name);
	tmp->state = L;

	if(luaL_dofile(L, path)) {
		LuaError(tmp->state);
		LuaPlugin_Close(tmp);
		LuaPlugin_Free(tmp);
		return false;
	}

	lua_getglobal(L, "API_VER");
	if(lua_isnil(L, -1) || !checkCompatibility((int)lua_tointeger(L, -1))) {
		Log_Warn("Plugin \"%s\" can't be loaded on this version of the server software", tmp->name);
		LuaPlugin_Close(tmp);
		LuaPlugin_Free(tmp);
		return false;
	} else {
		lua_pop(L, 1);
		tmp->next = headPlugin;
		if(headPlugin)
			headPlugin->prev = tmp;
		tmp->loaded = true;
		headPlugin = tmp;
		CallLuaVoid(tmp, "onStart");
	}

	return true;
}

#define PLUGLIST "List of loaded plugins:"
static void PrintList(CLIENT *client) {
	if(!client)
		Log_Info(PLUGLIST);
	else
		Packet_WriteChat(client, 0, PLUGLIST);

	PLUGIN *ptr = headPlugin;

	while(ptr) {
		if(!client)
			printf("%s\n", ptr->name);
		else
			Packet_WriteChat(client, 0, ptr->name);
		ptr = ptr->next;
	}
}

/*
	LuaPlugin commands
*/

static bool Cmd_Plugins(const char* args, CLIENT* caller, char* out) {
	char arg[64] = {0};

	if(String_GetArgument(args, arg, 64, 0)) {
		if(String_CaselessCompare(arg, "list")) {
			PrintList(caller);
			return false;
		} else if (String_CaselessCompare(arg, "load")) {
			if(String_GetArgument(args, arg, 64, 1)) {
				if(LuaPlugin_Load(arg)) {
					String_Copy(out, CMD_MAX_OUT, "Plugin loaded successfully");
				} else
					return false;
			} else {
				String_Copy(out, CMD_MAX_OUT, "Invalud argument #2");
			}
		} else if(String_CaselessCompare(arg, "unload")) {
			if(String_GetArgument(args, arg, 64, 1)) {
				PLUGIN* plugin = LuaPlugin_FindByName(arg);
				if(!plugin) {
					String_Copy(out, CMD_MAX_OUT, "This plugin is not loaded");
				} else {
					String_Copy(out, CMD_MAX_OUT, "Plugin unloading queued");
					plugin->loaded = false;
				}
			} else {
				String_Copy(out, CMD_MAX_OUT, "Invalud argument #2");
			}
		} else if(String_CaselessCompare(arg, "reload")) {
			if(String_GetArgument(args, arg, 64, 1)) {
				PLUGIN* plugin = LuaPlugin_FindByName(arg);
				if(!plugin) {
					String_Copy(out, CMD_MAX_OUT, "This plugin is not loaded");
				} else {
					String_Copy(out, CMD_MAX_OUT, "Plugin reloading queued");
					plugin->needReload = true;
					plugin->loaded = false;
				}
			} else {
				String_Copy(out, CMD_MAX_OUT, "Invalud argument #2");
			}
		}
	} else {
		String_Copy(out, CMD_MAX_OUT, "No arguments given");
	}

	return true;
}

static bool elp_onmessage(void* param) {
	onMessage_t* a = (onMessage_t*)param;
	PLUGIN* ptr = headPlugin;
	bool canSend = true;

	while(ptr) {
		if(ptr->loaded) {
			LuaCallback_Start(ptr, "onMessage");
			pushClient(ptr->state, a->client);
			lua_pushstring(ptr->state, a->message);
			if(!LuaCallback_Call(ptr, 2, 1)) {
				if(lua_isboolean(ptr->state, -1)) {
					if(!(canSend = lua_toboolean(ptr->state, -1)))
						break;
				}
			} else {
				canSend = false;
			}
			LuaCallback_End(ptr);
		}
		ptr = ptr->next;
	}

	return canSend;
}

static bool elp_onblockplace(void* param) {
	onBlockPlace_t* a = (onBlockPlace_t*)param;
	PLUGIN* ptr = headPlugin;
	bool canPlace = true;

	while(ptr) {
		if(ptr->loaded) {
			LuaCallback_Start(ptr, "onBlockPlace");
			pushClient(ptr->state, a->client);
			lua_pushinteger(ptr->state, *a->x);
			lua_pushinteger(ptr->state, *a->y);
			lua_pushinteger(ptr->state, *a->z);
			lua_pushinteger(ptr->state, *a->id);
			if(!LuaCallback_Call(ptr, 5, 1)) {
				if(lua_isboolean(ptr->state, -1)) {
					if(!(canPlace = lua_toboolean(ptr->state, -1)))
						break;
				}
			} else {
				canPlace = false;
			}
			LuaCallback_End(ptr);
		}
		ptr = ptr->next;
	}

	return canPlace;
}

static void elp_onhsdone(void* param) {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(ptr->loaded) {
			LuaCallback_Start(ptr, "onHandshakeDone");
			pushClient(ptr->state, param);
			LuaCallback_Call(ptr, 1, 0);
			LuaCallback_End(ptr);
		}
		ptr = ptr->next;
	}
}

static void elp_onspawn(void* param) {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(ptr->loaded) {
			LuaCallback_Start(ptr, "onSpawn");
			pushClient(ptr->state, param);
			LuaCallback_Call(ptr, 1, 0);
			LuaCallback_End(ptr);
		}
		ptr = ptr->next;
	}
}

static void elp_ondespawn(void* param) {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(ptr->loaded) {
			LuaCallback_Start(ptr, "onDespawn");
			pushClient(ptr->state, param);
			LuaCallback_Call(ptr, 1, 0);
			LuaCallback_End(ptr);
		}
		ptr = ptr->next;
	}
}

static void elp_onheldblockchange(void* param) {
	onHeldBlockChange_t* a = (onHeldBlockChange_t*)param;
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(ptr->loaded) {
			LuaCallback_Start(ptr, "onHeldBlockChange");
			pushClient(ptr->state, a->client);
			lua_pushinteger(ptr->state, *a->prev);
			lua_pushinteger(ptr->state, *a->curr);
			LuaCallback_Call(ptr, 3, 0);
			LuaCallback_End(ptr);
		}
		ptr = ptr->next;
	}
}

void LuaPlugin_Start() {
	Command_Register("plugins", Cmd_Plugins, true);
	Event_RegisterBool(EVT_ONMESSAGE, elp_onmessage);
	Event_RegisterBool(EVT_ONBLOCKPLACE, elp_onblockplace);
	Event_RegisterVoid(EVT_ONHANDSHAKEDONE, elp_onhsdone);
	Event_RegisterVoid(EVT_ONSPAWN, elp_onspawn);
	Event_RegisterVoid(EVT_ONDESPAWN, elp_ondespawn);
	Event_RegisterVoid(EVT_ONHELDBLOCKCHNG, elp_onheldblockchange);

	Directory_Ensure("plugins");
	dirIter pIter = {0};
	if(Iter_Init(&pIter, "plugins", "lua")) {
		do {
			if(pIter.isDir || !pIter.cfile) continue;
			LuaPlugin_Load(pIter.cfile);
		} while(Iter_Next(&pIter));
	} else
		Log_FormattedError();
	Iter_Close(&pIter);
}

PLUGIN* LuaPlugin_FindByName(const char* name) {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(ptr->loaded && String_Compare(name, ptr->name)) {
			return ptr;
		}
		ptr = ptr->next;
	}

	return (PLUGIN*)NULL;
}

void LuaPlugin_Remove(PLUGIN* plugin) {
	if(plugin->prev)
		plugin->prev->next = plugin->next;
	else
		headPlugin = NULL;

	if(plugin->next)
		plugin->next->prev = plugin->prev;
}

void LuaPlugin_Tick() {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(ptr->loaded) {
			CallLuaVoid(ptr, "onTick");
		} else {
			LuaPlugin_Close(ptr);
			LuaPlugin_Remove(ptr);
			if(ptr->needReload) {
				LuaPlugin_Load(ptr->name);
			}
			LuaPlugin_Free(ptr);
		}
		ptr = ptr->next;
	}
}

void LuaPlugin_Free(PLUGIN* plugin) {
	Memory_Free((void*)plugin->name);
	Memory_Free(plugin);
}

void LuaPlugin_Close(PLUGIN* plugin) {
	CallLuaVoid(plugin, "onStop");
	lua_close(plugin->state);
}

void LuaPlugin_Stop() {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		LuaPlugin_Close(ptr);
		LuaPlugin_Free(ptr);
		ptr = ptr->next;
	}
}
#endif
