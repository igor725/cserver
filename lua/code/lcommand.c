#include <core.h>
#include <str.h>
#include <command.h>

#include "script.h"
#include "lclient.h"

static cs_bool lua_cmd_handler(CommandCallData ccdata) {
	Script scr = ccdata->command->data;
	Command cmd = ccdata->command;

	Mutex_Lock(scr->mutex);
	lua_pushlightuserdata(scr->state, cmd);
	lua_gettable(scr->state, LUA_REGISTRYINDEX);
	if(lua_isfunction(scr->state, -1)) {
		lua_pushstring(scr->state, ccdata->args);
		luax_pushclient(scr->state, ccdata->caller);
		if(lua_pcall(scr->state, 2, 1, 0)) {
			String_FormatBuf(ccdata->out, MAX_CMD_OUT, "Lua error: %s.", lua_tostring(scr->state, -1));
			Mutex_Unlock(scr->mutex);
			return true;
		}
		if(lua_isstring(scr->state, -1)) {
			String_Copy(ccdata->out, MAX_CMD_OUT, lua_tostring(scr->state, -1));
			Mutex_Unlock(scr->mutex);
			return true;
		}
	} else {
		Mutex_Unlock(scr->mutex);
		Command_Printf(ccdata, "Strange lua error: Eh? Where is callback for \"%s\" command????", cmd->name);
	}

	Mutex_Unlock(scr->mutex);
	return false;
}

static cs_int32 c_register(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	Command cmd = Command_Register(name, lua_cmd_handler);
	cmd->data = Script_GetByState(L);

	lua_pushlightuserdata(L, cmd);
	lua_pushvalue(L, 2);
	lua_settable(L, LUA_REGISTRYINDEX);

	return 0;
}

static cs_int32 c_unregister(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	Command cmd = Command_Get(name);
	if(cmd && cmd->data == Script_GetByState(L)) {
		lua_pushlightuserdata(L, cmd);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);

		lua_pushboolean(L, Command_Unregister(cmd));
	} else lua_pushboolean(L, false);

	return 1;
}

static const luaL_Reg cmdfuncs[] = {
  {"register", c_register},
  {"unregister", c_unregister},

  {NULL, NULL}
};

cs_int32 luaopen_command(lua_State* L) {
	luaL_newlib(L, cmdfuncs);
	return 1;
}
