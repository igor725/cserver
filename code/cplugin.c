#ifdef CP_ENABLED
#include "core.h"
#include "event.h"
#include "client.h"
#include "server.h"
#include "cplugin.h"

bool CPlugin_Load(const char* name) {
	char path[256];
	char error[512];
	String_FormatBuf(path, 256, "plugins/%s", name);
	void* plugin;
	void* verSym;
	void* initSym;
	int ver;

	if(DLib_Load(path, &plugin)) {
		if(!(DLib_GetSym(plugin, "Plugin_ApiVer", &verSym) &&
		DLib_GetSym(plugin, "Plugin_Init", &initSym))) {
			Log_Error("%s: %s", path, DLib_GetError(error, 512));
			DLib_Unload(plugin);
			return false;
		}

		ver = *((int*)verSym);
		if(ver / 100 != CPLUGIN_API_NUM / 100) {
			if(ver < CPLUGIN_API_NUM)
				Log_Error(CPLUGIN_OLDMSG, name, CPLUGIN_API_NUM, ver);
			else
				Log_Error(CPLUGIN_UPGMSG, name, ver, CPLUGIN_API_NUM);

			DLib_Unload(plugin);
			return false;
		}
		return (*(initFunc)initSym)();
	}

	Log_Error("%s: %s", path, DLib_GetError(error, 512));
	return false;
}

void CPlugin_Start() {
	Directory_Ensure("plugins");
	dirIter pIter = {0};
	if(Iter_Init(&pIter, "plugins", DLIB_EXT)) {
		do {
			if(pIter.isDir || !pIter.cfile) continue;
			if(CPlugin_Load(pIter.cfile))
				Log_Info("Plugin %s loaded", pIter.cfile);
		} while(Iter_Next(&pIter));

	}
}
#endif
