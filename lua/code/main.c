// Build command: build [dbg] pb lua [install] lua53
#include <core.h>
#include <str.h>
#include <log.h>
#include <event.h>

#define LUA_LIB
#define LUA_BUILD_AS_DLL
#include "script.h"
#include "lclient.h"

void* luax_newobject(lua_State* L, cs_size size, const char* mt) {
	void* ptr = lua_newuserdata(L, size);
	luaL_setmetatable(L, mt);
	return ptr;
}

void* luax_checkobject(lua_State* L, cs_int32 idx, const char* mt) {
	return luaL_checkudata(L, idx, mt);
}

void* luax_checkptr(lua_State* L, cs_int32 idx, const char* mt) {
	return *(void**)luaL_checkudata(L, idx, mt);
}

void luax_pushptr(lua_State* L, void* obj, const char* mt, uptrSetupFunc setup) {
	if(obj == NULL) {
		lua_pushnil(L);
		return;
	}
	lua_pushlightuserdata(L, obj);
	lua_gettable(L, LUA_REGISTRYINDEX);

	if(lua_isnil(L, -1)) {
		lua_pop(L, 1);
		*(void**)lua_newuserdata(L, sizeof(cs_uintptr)) = obj;
		luaL_setmetatable(L, mt);
		if(setup) setup(L, obj);

		lua_pushlightuserdata(L, obj);
		lua_pushvalue(L, -2);

		lua_settable(L, LUA_REGISTRYINDEX);
	}
}

void luax_pushrgtableof(lua_State* L, cs_int32 idx) {
	lua_getfield(L, LUA_REGISTRYINDEX, "cs_udata");

	if(idx < 0) idx--;
	lua_pushvalue(L, idx);
	lua_gettable(L, -2);
}

void luax_destroyptr(lua_State* L, void** upp, cs_bool free) {
	void* up = *upp;
	if(free) Memory_Free(up);
	lua_pushlightuserdata(L, up);
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);
	*upp = NULL;
}

LUA_FUNC(lptr_gc) {
	luax_destroyptr(L, (void**)lua_touserdata(L, 1), false);
	return 0;
}

LUA_FUNC(luax_printstack) {
	Log_Info("Stack start:");
  for (int i = 0; i <= lua_gettop(L); i++) {
    int t = lua_type(L, i);

    switch (t) {
	    case LUA_TSTRING:
				Log_Info("   %d - %s: %s", i, luaL_typename(L, i), lua_tostring(L, i));
				break;
	    case LUA_TBOOLEAN:
				Log_Info("   %d - %s: %s", i, luaL_typename(L, i), (lua_toboolean(L, i) ? "true" : "false"));
				break;
	    case LUA_TNUMBER:
				Log_Info("   %d - %s: %d", i, luaL_typename(L, i), lua_tonumber(L, i));
				break;
			default:
				Log_Info("   %d - %s", i, luaL_typename(L, i));
    }
  }
	return 0;
}

Script* scriptHead = NULL;

static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
	{LUA_IOLIBNAME, luaopen_io},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},

	{"server", luaopen_server},
	{"vector", luaopen_csmath},
	{"client", luaopen_client},
	{"world", luaopen_world},
	{"command", luaopen_command},
	// {"config", luaopen_config},

  {NULL, NULL}
};

static void OpenLibs(lua_State *L) {
  const luaL_Reg *lib;
  for (lib = loadedlibs; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);
  }

	lua_getglobal(L, "package");
	lua_pushstring(L, "./scripts/libs/?.lua;./scripts/libs/?/init.lua");
	lua_setfield(L, -2, "path");
	const char* cpath = NULL;
#if defined(WINDOWS)
	cpath = "./scripts/bin/?.dll;./scripts/bin/?/core.dll";
#elif defined(POSXI)
	cpath = "./scripts/bin/?.so;./scripts/bin/?/core.so";
#endif
	lua_pushstring(L, cpath);
	lua_setfield(L, -2, "cpath");
	lua_pop(L, 1);

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "cs_udata");
}

void Script_Open(const char* name) {
	char path[256];
	String_FormatBuf(path, 256, "scripts/%s", name);

	lua_State* state = luaL_newstate();
	OpenLibs(state);
	if(luaL_dofile(state, path)) {
		Log_Error("Can't load Lua script: %s.", lua_tostring(state, -1));
		lua_close(state);
		return;
	}

	Script* scr = Memory_Alloc(1, sizeof(Script));
	scr->state = state;
	scr->mutex = Mutex_Create();
	scr->next = scriptHead;
	scr->name = String_AllocCopy(name);
	scriptHead = scr;

	Script_GetFuncBegin(scr, "onStart");
		cs_int32 ret = lua_pcall(L, 0, 0, 0);
		if(ret) Log_Error("LuaVM error in \"%s\": %s.", scr->name, lua_tostring(L, -1));
	Script_GetFuncEnd(scr);
}

Script* Script_GetByState(lua_State* L) {
	Script* ptr = scriptHead;

	while(ptr) {
		if(ptr->state == L)
			return ptr;
		ptr = ptr->next;
	}

	return NULL;
}

void Script_Destroy(Script* scr) {
	Script_GetFuncBegin(scr, "onStop");
		cs_int32 ret = lua_pcall(L, 0, 0, 0);
		if(ret) Log_Error("LuaVM error can't call onStop \"%s\": %s.", scr->name, lua_tostring(L, -1));
	Script_GetFuncEnd(scr);

	Mutex_Lock(scr->mutex);
	scr->destroyed = true;
	lua_close(scr->state);
	Mutex_Unlock(scr->mutex);
	Mutex_Free(scr->mutex);
	Memory_Free((void*)scr->name);
	Memory_Free(scr);
}

#define BeginLuaEventCall(ename) \
Script* ptr = scriptHead; \
while(ptr) { \
	Script_GetFuncBegin(ptr, ename);

#define EndLuaEventCall \
	Script_GetFuncEnd(ptr); \
	ptr = ptr->next; \
}

static void scrtick(void* param) {
	cs_int32 delta = *(cs_int32*)param;
	BeginLuaEventCall("onTick");
		lua_pushinteger(L, delta);
		if(lua_pcall(L, 1, 0, 0)) {
			Log_Error("LuaVM error \"%s\": %s.", ptr->name, lua_tostring(L, -1));
			ptr->stopped = true;
		}
	EndLuaEventCall;
}

static void scrhshake(void* param) {
	BeginLuaEventCall("onConnect");
		luax_pushclient(L, param);
		if(lua_pcall(L, 1, 0, 0)) {
			Log_Error("LuaVM error \"%s\": %s.", ptr->name, lua_tostring(L, -1));
			ptr->stopped = true;
		}
	EndLuaEventCall;
}

static void scrdisc(void* param) {
	BeginLuaEventCall("onDisconnect");
		luax_pushclient(L, param);
		if(lua_pcall(L, 1, 0, 0)) {
			Log_Error("LuaVM error \"%s\": %s.", ptr->name, lua_tostring(L, -1));
			ptr->stopped = true;
		}
	EndLuaEventCall;
}

// static void scrmsg(void* param) {
// 	onMessage a = (onMessage)param;
// 	BeginLuaEventCall("onMessage");
// 		luax_pushclient(L, a->client);
// 		lua_pushstring(L, a->message);
// 		lua_pushinteger(L, *a->type);
// 		if(lua_pcall(L, 3, 1, 0)) {
// 			Log_Error("LuaVM error \"%s\": %s.", ptr->name, lua_tostring(L, -1));
// 			ptr->stopped = true;
// 		} else {
// 			if(lua_isinteger(L, -1)) {
// 				*a->type = (MessageType)lua_tointeger(L, -1);
// 				break;
// 			}
// 		}
// 	EndLuaEventCall;
// }

Plugin_SetVersion(1);

cs_bool Plugin_Load() {
	dirIter scIter;
	Directory_Ensure("scripts");

	Event_RegisterVoid(EVT_ONTICK, scrtick);
	Event_RegisterVoid(EVT_ONHANDSHAKEDONE, scrhshake);
	Event_RegisterVoid(EVT_ONDISCONNECT, scrdisc);
	// Event_RegisterVoid(EVT_ONMESSAGE, scrmsg);

	if(Iter_Init(&scIter, "scripts", "lua")) {
		do {
			if(!scIter.isDir && scIter.cfile)
				Script_Open(scIter.cfile);
		} while(Iter_Next(&scIter));
	}
	Iter_Close(&scIter);
	return true;
}

cs_bool Plugin_Unload() {
	Script* ptr = scriptHead, *tmp;

	while(ptr) {
		tmp = ptr;
		ptr = ptr->next;
		Script_Destroy(tmp);
	}

	return true;
}
