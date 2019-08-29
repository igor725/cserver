#ifdef LUA_ENABLED
#include "core.h"
#include "log.h"
#include "luaplugin.h"

// static const luaL_Reg loglib[] = {
// 	{"info", },
// 	{"error", },
// 	{"warn", },
// };

int luaopen_log(lua_State* L) {
	luaL_register(L, lua_tostring(L, -1), loglib);
	return 0;
}

int luaopen_event(lua_State* L) {
	return 0;
}

inv luaopen_config(lua_State* L) {
	return 0;
}

int luaopen_cpe(lua_State* L) {
	return 0;
}

int luaopen_packets(lua_State* L) {
	return 0;
}

int luaopen_world(lua_State* L) {
	return 0;
}

static const luaL_Reg LuaPlugin_Libs[] = {
	{"", luaopen_base},
	{"log", luaopen_log},
	{"event", luaopen_event},
	{"config", luaopen_config},
	{"cpe", luaopen_cpe},
	{"packets", luaopen_packets},
	{"world", luaopen_world},
	{LUA_TABLIBNAME, luaopen_table},
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{LUA_DBLIBNAME, luaopen_debug},
	{NULL, NULL}
};

void LuaPlugin_LoadLibs(lua_State* L) {
	const luaL_Reg *lib;
	for(lib = LuaPlugin_Libs; lib->func; lib++) {
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}
}

static void PrintError(LUAPLUGIN* plugin) {
	Log_Error(lua_tostring(plugin->state, -1));
}

bool LuaPlugin_Load(const char* name) {
	char path[MAX_PATH];
	String_FormatBuf(path, MAX_PATH, "plugins/%s.lua", name);

	lua_State* L = luaL_newstate();
	LuaPlugin_LoadLibs(L);
	LUAPLUGIN* tmp = Memory_Alloc(1, sizeof(struct luaPlugin));
	tmp->state = L;
	tmp->name = name;

	if(luaL_dofile(L, path)) {
		PrintError(tmp);
		lua_close(L);
		free(tmp);
		return false;
	} else {
		tmp->next = headPlugin;
		headPlugin = tmp;
	}

	return true;
}

void LuaPlugin_Start() {
	LuaPlugin_Load("test");
}

void LuaPlugin_Close(LUAPLUGIN* plugin) {
	// TODO: Call onClose() function from plugin if it exists
	lua_close(plugin->state);
	free(plugin);
}

void LuaPlugin_Stop() {
	LUAPLUGIN* ptr = headPlugin;
	while(ptr) {
		LuaPlugin_Close(ptr);
		ptr = ptr->next;
	}
}
#endif
