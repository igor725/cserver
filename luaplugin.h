#ifndef LUAPLUGIN_H
#define LUAPLUGIN_H
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define	CFG_BOOL_NAME "boolean"
#define	CFG_INT_NAME  "number"
#define	CFG_STR_NAME  "string"
#define	CFG_UNK_NAME  "unknown"

typedef struct luaPlugin {
	const char* name;
	lua_State* state;
	struct luaPlugin* next;
} PLUGIN;

bool LuaPlugin_Load(const char* name);
void LuaPlugin_Close(PLUGIN* plugin);
PLUGIN* LuaPlugin_GetPluginByState(lua_State* L);
void LuaPlugin_Start();
void LuaPlugin_Stop();
PLUGIN* headPlugin;
#endif
