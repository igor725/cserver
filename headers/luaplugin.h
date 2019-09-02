#ifndef LUAPLUGIN_H
#define LUAPLUGIN_H
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#if LUA_VERSION_NUM != 501
#error This version of Lua intepreter currently is not supported.
#endif

#define	CFG_BOOL_NAME "boolean"
#define	CFG_INT_NAME  "number"
#define	CFG_STR_NAME  "string"
#define	CFG_UNK_NAME  "unknown"

#define LPLUGIN_API_NUM 100

typedef struct luaPlugin {
	const char* name;
	lua_State* state;
	bool loaded;
	struct luaPlugin* prev;
	struct luaPlugin* next;
} PLUGIN;

bool LuaPlugin_Load(const char* name);
void LuaPLugin_Unload(PLUGIN* plugin);
void LuaPlugin_Close(PLUGIN* plugin);
void LuaPlugin_Free(PLUGIN* plugin);
PLUGIN* LuaPlugin_FindByName(const char* name);
PLUGIN* LuaPlugin_FindByState(lua_State* L);
void LuaPlugin_Start();
void LuaPlugin_Tick();
void LuaPlugin_Stop();
PLUGIN* headPlugin;
#endif
