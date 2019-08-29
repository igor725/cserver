#ifndef LUAPLUGIN_H
#define LUAPLUGIN_H
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

void LuaPlugin_Close();
void LuaPlugin_Start();
lua_State* LuaPlugin_State;
#endif
