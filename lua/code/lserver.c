#include <core.h>
#include <platform.h>
#include <server.h>

#include "script.h"

LUA_SFUNC(fserver_stop) {
	(void)L;
	Server_Active = false;
	return 0;
}

LUA_SFUNC(fserver_currtime) {
	lua_pushinteger(L, Time_GetMSec());
	return 1;
}

static const luaL_Reg sv_funcs[] = {
	{"stop", fserver_stop},

	{"currtime", fserver_currtime},

	{NULL, NULL}
};

LUA_FUNC(luaopen_server) {
	luaL_newlib(L, sv_funcs);
	lua_pushinteger(L, (lua_Integer)Server_StartTime);
	lua_setfield(L, -2, "starttime");
	return 1;
}
