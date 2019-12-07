#ifndef SCRIPT_H
#define SCRIPT_H
#include <platform.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define LUA_FUNC(N) \
cs_int32 N(lua_State* L)

#define LUA_APIFUNC(N) \
LUA_API LUA_FUNC(N)

#define LUA_SFUNC(N) \
static LUA_FUNC(N)

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

void* luax_checkmyobject(lua_State* L, cs_uint32 idx, const char* mt);
void luax_pushmyobject(lua_State* L, void* obj, const char* mt);
void luax_destroymyobject(lua_State* L, void** upp, cs_bool free);
void luax_printstack(lua_State* L);
LUA_FUNC(lsuper_gc);

Script Script_GetByState(lua_State* L);
void Script_Destroy(Script scr);

LUA_APIFUNC(luaopen_server);
// LUA_APIFUNC(luaopen_vector);
LUA_APIFUNC(luaopen_client);
LUA_APIFUNC(luaopen_world);
LUA_APIFUNC(luaopen_command);
LUA_APIFUNC(luaopen_config);
#endif
