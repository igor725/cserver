#ifdef LUA_ENABLED
#include "luaplugin.h"

void LuaPlugin_Start() {
	LuaPlugin_State = luaL_newstate();
}

void LuaPlugin_Close() {
	lua_close(LuaPlugin_State);
	LuaPlugin_State = NULL;
}
#endif
