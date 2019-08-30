#ifndef LUAPLUGIN_H
#define LUAPLUGIN_H
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

typedef struct luaPlugin {
	const char* name;
	lua_State* state;
	struct luaPlugin* next;
} LUAPLUGIN;

bool LuaPlugin_Load(const char* name);
void LuaPlugin_Close(LUAPLUGIN* plugin);
LUAPLUGIN* LuaPlugin_GetPluginByState(lua_State* L);
void LuaPlugin_Start();
void LuaPlugin_Stop();
LUAPLUGIN* headPlugin;
#endif
