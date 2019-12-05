// Build command: build [dbg] pb lua [install] lua53
#include <core.h>
#include <str.h>
#include <log.h>

#define LUA_LIB
#define LUA_BUILD_AS_DLL
#include "script.h"

void* luax_checkmyobject(lua_State* L, cs_uint32 idx, const char* mt) {
	return *(void**)luaL_checkudata(L, idx, mt);
}

void luax_pushmyobject(lua_State* L, void* obj, const char* mt) {
	lua_pushlightuserdata(L, obj);
	lua_gettable(L, LUA_REGISTRYINDEX);

	if(lua_isnil(L, -1)) {
		lua_pop(L, 1);
		*(void**)lua_newuserdata(L, sizeof(cs_uintptr)) = obj;
		luaL_setmetatable(L, mt);

		lua_pushlightuserdata(L, obj);
		lua_pushvalue(L, -2);

		lua_settable(L, LUA_REGISTRYINDEX);
	}
}

void luax_printstack(lua_State* L) {
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
}

Script scriptHead = NULL;

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

	// {"vector", luaopen_vector},
	{"client", luaopen_client},
	{"world", luaopen_world},
	{"command", luaopen_command},

  {NULL, NULL}
};


static void OpenLibs(lua_State *L) {
  const luaL_Reg *lib;
  for (lib = loadedlibs; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);
  }
}

void Script_CallStart(Script scr) {
	Script_GetFuncBegin(scr, "onStart");
		cs_int32 ret = lua_pcall(L, 0, 0, 0);
		if(ret) Log_Error("LuaVM error in \"%s\": %s.", scr->name, lua_tostring(L, -1));
	Script_GetFuncEnd(scr);
}

void Script_CallStop(Script scr) {
	Script_GetFuncBegin(scr, "onStop");
		cs_int32 ret = lua_pcall(L, 0, 0, 0);
		if(ret) Log_Error("LuaVM error can't call onStop \"%s\": %s.", scr->name, lua_tostring(L, -1));
	Script_GetFuncEnd(scr);
}

void Script_Open(const char* name) {
	char path[256];
	String_FormatBuf(path, 256, "scripts/%s", name);

	lua_State* L = luaL_newstate();
	OpenLibs(L);
	if(luaL_dofile(L, path)) {
		Log_Error("Can't load Lua script: %s.", lua_tostring(L, -1));
		lua_close(L);
		return;
	}

	Script scr = Memory_Alloc(1, sizeof(struct _Script));
	scr->state = L;
	scr->mutex = Mutex_Create();
	scr->next = scriptHead;
	scr->name = String_AllocCopy(name);
	scriptHead = scr;

	Script_CallStart(scr);
}

Script Script_GetByState(lua_State* L) {
	Script ptr = scriptHead;

	while(ptr) {
		if(ptr->state == L)
			return ptr;
		ptr = ptr->next;
	}

	return NULL;
}

void Script_Destroy(Script scr) {
	Script_CallStop(scr);

	Mutex_Lock(scr->mutex);
	scr->destroyed = true;
	lua_close(scr->state);
	Mutex_Unlock(scr->mutex);
	Mutex_Free(scr->mutex);
	Memory_Free((void*)scr->name);
	Memory_Free(scr);
}

cs_int32 Plugin_ApiVer = PLUGIN_API_NUM;

cs_bool Plugin_Load() {
	dirIter scIter;
	Directory_Ensure("scripts");
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
	Script ptr = scriptHead, tmp;

	while(ptr) {
		tmp = ptr;
		ptr = ptr->next;
		Script_Destroy(tmp);
	}

	return true;
}
