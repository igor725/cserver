#include <core.h>
#include <config.h>
#include <server.h>

#include "svhttp.h"

CFGSTORE cplStore;
bool loaded = false;

EXP int Plugin_ApiVer = 100;

EXP bool Plugin_Load(void) {
	cplStore = Config_Create("cp.cfg");
	if(!cplStore || !Server_Config) return false;

	Config_SetBool(cplStore, "enabled", true);
	Config_SetInt(cplStore, "port", 8080);
	if(!Config_Load(cplStore)) return false;

	if(Config_GetBool(cplStore, "enabled")) {
		const char* ip = Config_GetStr(Server_Config, "ip");
		uint16_t port = Config_GetInt(cplStore, "port");

		if(Http_StartServer(ip, port))
			Log_Info("CPL http server started on %s:%d", ip, port);
		else
			return false;

		loaded = true;
		return true;
	}

	return false;
}

EXP bool Plugin_Unload(void) {
	Http_CloseServer();
	if(cplStore) {
		if(loaded) Config_Save(cplStore);
		Config_EmptyStore(cplStore);
		Memory_Free(cplStore);
	}
	return true;
}
