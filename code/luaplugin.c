#ifdef LUA_ENABLED
#include "core.h"
#include "log.h"
#include "config.h"
#include "luaplugin.h"
#include "command.h"
#include "packets.h"

static int LuaError(lua_State* L) {
	PLUGIN* plugin = LuaPlugin_FindByState(L);
	if(plugin)
		Log_Error("In plugin \"%s\": %s", plugin->name, lua_tostring(L, -1));
	else
		Log_Error(lua_tostring(L, -1));
	return 0;
}

#define CallLuaVoid(plugin, func) \
lua_pushcfunction(plugin->state, LuaError); \
lua_getglobal(plugin->state, func); \
if(lua_isfunction(plugin->state, -1)) \
	lua_pcall(plugin->state, 0, 0, -2); \
else \
	lua_pop(plugin->state, 1); \

#define GetLuaPlugin(L) \
PLUGIN* plugin = LuaPlugin_FindByState(L); \
if(!plugin) \
	return 0; \

/*
	Lua log library
*/

static int llog_info(lua_State* L) {
	GetLuaPlugin(L);

	const char* str = lua_tostring(L, 1);
	if(str)
		Log_Info("%s: %s", plugin->name, str);
	return 0;
}

static int llog_error(lua_State* L) {
	GetLuaPlugin(L);

	const char* str = lua_tostring(L, 1);
	if(str)
		Log_Error("%s: %s", plugin->name, str);
	return 0;
}

static int llog_warn(lua_State* L) {
	GetLuaPlugin(L);

	const char* str = lua_tostring(L, 1);
	if(str)
		Log_Warn("%s: %s", plugin->name, str);
	return 0;
}

static const luaL_Reg loglib[] = {
	{"info", llog_info},
	{"error", llog_error},
	{"warn", llog_warn},
	{NULL, NULL}
};

int luaopen_log(lua_State* L) {
	luaL_register(L, lua_tostring(L, -1), loglib);
	return 1;
}

/*
	Lua event library
*/

int luaopen_event(lua_State* L) {
	return 0;
}

/*
	Lua client library
*/

#define LUA_TCLIENT "client"

static CLIENT* toClient(lua_State* L, int index) {
	CLIENT* client = lua_touserdata(L, index);
	if(!client) luaL_typerror(L, index, LUA_TCLIENT);
	return client;
}

static void pushClient(lua_State* L, CLIENT* client) {
	lua_pushlightuserdata(L, client);
	luaL_getmetatable(L, LUA_TCLIENT);
	lua_setmetatable(L, -2);
}

static int lclient_get(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	CLIENT* client = Client_FindByName(name);

	if(!client)
		lua_pushnil(L);
	else
		pushClient(L, client);

	return 1;
}

static int lclient_kick(lua_State* L) {
	CLIENT* client = toClient(L, 1);
	const char* reason = luaL_checkstring(L, 2);
	bool succ = false;

	if(!client) {
		succ = false;
	}

	succ = true;
	Client_Kick(client, reason);

	lua_pushboolean(L, succ);
	return 1;
}

static const luaL_Reg client_methods[] = {
	{"get", lclient_get},
	{"kick", lclient_kick},
	{NULL, NULL}
};

static int lclient_tostring(lua_State* L) {
	CLIENT* client = toClient(L, 1);
	if(!client->playerData)
		lua_pushstring(L, "unconnected");
	else
		lua_pushstring(L, client->playerData->name);
	return 1;
}

static const luaL_Reg client_meta[] = {
	{"__tostring", lclient_tostring},
	{NULL, NULL}
};

int luaopen_client(lua_State* L) {
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

#define LUA_TCFGSTORE "cfgStore"

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
	return 1;
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
			lua_pushnil(L);
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

int luaopen_config(lua_State* L) {
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

static const luaL_Reg packetslib[] = {
	{NULL, NULL}
};

int luaopen_packets(lua_State* L) {
	luaL_register(L, lua_tostring(L, -1), packetslib);
	return 1;
}

/*
	Lua world library
*/

#define LUA_TWORLD "classicWorld"

static WORLD* toWorld(lua_State* L, int index) {
	WORLD* world = lua_touserdata(L, index);
	if(!world) luaL_typerror(L, index, LUA_TWORLD);
	return world;
}

void pushWorld(lua_State* L, WORLD* world) {
	lua_pushlightuserdata(L, world);
	luaL_getmetatable(L, LUA_TWORLD);
	lua_setmetatable(L, -2);
}

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

int luaopen_world(lua_State* L) {
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

static const luaL_Reg LuaPlugin_Libs[] = {
	{"", luaopen_base},
	{"log", luaopen_log},
	{"event", luaopen_event},
	{"client", luaopen_client},
	{"config", luaopen_config},
	{"packets", luaopen_packets},
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
	PLUGIN* pl = LuaPlugin_FindByState(L);
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

	/*
	TODO: Убрать комментарий, когда появится возможность
	отправки vararg в функцию log.info
	P.S. Я действительно надеюсь, что это когда-нибудь
	случится

	lua_getglobal(L, "log");
	lua_getfield(L, -1, "info");
	lua_setglobal(L, "print");
	lua_pop(L, 1);
	*/
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

	lua_getglobal(L, "svVerNum");
	if(lua_isnil(L, -1) || lua_tointeger(L, -1) != SOFTWARE_VERSION_NUM) {
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
				}
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
		}
	} else {
		String_Copy(out, CMD_MAX_OUT, "No arguments given :(");
	}

	return true;
}

void LuaPlugin_Start() {
	Command_Register("plugins", &Cmd_Plugins, true);

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
		if(String_Compare(name, ptr->name)) {
			return ptr;
		}
		ptr = ptr->next;
	}

	return (PLUGIN*)NULL;
}

PLUGIN* LuaPlugin_FindByState(lua_State* L) {
	PLUGIN* ptr = headPlugin;

	while(ptr) {
		if(ptr->state == L) {
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
