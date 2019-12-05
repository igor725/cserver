#include <core.h>
#include <server.h>

#include "script.h"

LUA_SFUNC(lserver_stop) {
	(void)L;
	Server_Active = false;
	return 0;
}

LUA_SFUNC(lserver_currtime) {
	lua_pushinteger(L, Time_GetMSec());
	return 1;
}

static const luaL_Reg serverfuncs[] = {
	{"stop", lserver_stop},

	{"currtime", lserver_currtime},

	{NULL, NULL}
};

LUA_FUNC(luaopen_server) {
	luaL_newlib(L, serverfuncs);
	lua_pushinteger(L, (lua_Integer)Server_StartTime);
	lua_setfield(L, -2, "starttime");
	return 1;
}
