#ifndef SCRIPT_H
#define SCRIPT_H
#include <platform.h>

#if defined(WINDOWS)
#define LUA_LIB
#define LUA_BUILD_AS_DLL
#endif

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
if(!scr->destroyed && !scr->stopped) { \
	Mutex_Lock(scr->mutex); \
	lua_State* L = scr->state; \
	lua_getglobal(L, funcName); \
	if(lua_isfunction(L, -1)) {

#define Script_GetFuncEnd(scr) \
	} else { \
		lua_pop(L, 1); \
	} \
	Mutex_Unlock(scr->mutex); \
}

typedef struct _Script {
	cs_bool destroyed, stopped;
	const char* name;
	Mutex* mutex;
	lua_State* state;
	struct _Script* next;
} Script;

typedef void(*uptrSetupFunc)(lua_State* L, void* obj);

void* luax_newobject(lua_State* L, cs_size size, const char* mt);
void* luax_checkobject(lua_State* L, cs_int32 idx, const char* mt);
void luax_pushrgtableof(lua_State* L, cs_int32 idx);

void* luax_checkptr(lua_State* L, cs_int32 idx, const char* mt);
void luax_pushptr(lua_State* L, void* obj, const char* mt, uptrSetupFunc setup);
void luax_destroyptr(lua_State* L, void** upp, cs_bool free);

LUA_FUNC(luax_printstack);
LUA_FUNC(lptr_gc);

Script* Script_GetByState(lua_State* L);
void Script_Destroy(Script* scr);

LUA_APIFUNC(luaopen_server);
LUA_APIFUNC(luaopen_csmath);
LUA_APIFUNC(luaopen_client);
LUA_APIFUNC(luaopen_world);
LUA_APIFUNC(luaopen_command);
LUA_APIFUNC(luaopen_config);
#endif
