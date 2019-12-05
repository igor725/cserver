#ifndef SCRIPT_H
#define SCRIPT_H
#include <platform.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define Script_GetFuncBegin(scr, funcName) \
if(scr->destroyed) return; \
Mutex_Lock(scr->mutex); \
lua_State* L = scr->state; \
lua_getglobal(L, funcName); \
if(lua_isfunction(L, -1)) {

#define Script_GetFuncEnd(scr) \
} else { \
	lua_pop(L, 1); \
} \
Mutex_Unlock(scr->mutex);

typedef struct _Script {
	cs_bool destroyed;
	const char* name;
	Mutex* mutex;
	lua_State* state;
	struct _Script* next;
} *Script;

// Служебные функции
void* luax_checkmyobject(lua_State* L, cs_uint32 idx, const char* mt);
void luax_pushmyobject(lua_State* L, void* obj, const char* mt);
void luax_printstack(lua_State* L);

LUA_API cs_int32 luaopen_server(lua_State* L);
LUA_API cs_int32 luaopen_protocol(lua_State* L);
LUA_API cs_int32 luaopen_command(lua_State* L);
LUA_API cs_int32 luaopen_client(lua_State* L);
LUA_API cs_int32 luaopen_world(lua_State* L);
LUA_API cs_int32 luaopen_config(lua_State* L);

Script Script_GetByState(lua_State* L);
void Script_Destroy(Script scr);
#endif
