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
		DLib_GetSym(plugin, "Plugin_Load", &initSym))) {
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

		CPLUGIN* splugin = (CPLUGIN*)Memory_Alloc(1, sizeof(CPLUGIN));
		splugin->name = String_AllocCopy(name);
		splugin->lib = plugin;
		DLib_GetSym(plugin, "Plugin_Unload", (void*)&splugin->unload);
		int pluginId = -1;
		for(int i = 0; i < MAX_PLUGINS; i++) {
			if(!pluginsList[i]) {
				pluginsList[i] = splugin;
				pluginId = i;
				break;
			}
		}
		splugin->id = pluginId;
		if(pluginId == -1 || !(*(pluginFunc)initSym)())
			CPlugin_Unload(splugin);

		return false;
	}

	Log_Error("%s: %s", path, DLib_GetError(error, 512));
	return false;
}

bool CPlugin_Unload(CPLUGIN* plugin) {
	if(plugin->unload && !(*(pluginFunc)plugin->unload)())
		return false;

	if(plugin->name)
		Memory_Free((void*)plugin->name);
	if(plugin->id != -1)
		pluginsList[plugin->id] = NULL;

	DLib_Unload(plugin->lib);
	Memory_Free(plugin);
	return true;
}

void CPlugin_Start() {
	Directory_Ensure("plugins");
	dirIter pIter = {0};
	if(Iter_Init(&pIter, "plugins", DLIB_EXT)) {
		do {
			if(pIter.isDir || !pIter.cfile) continue;
			CPlugin_Load(pIter.cfile);
		} while(Iter_Next(&pIter));
	}
}

void CPlugin_Stop() {
	for(int i = 0; i < MAX_PLUGINS; i++) {
		CPLUGIN* plugin = pluginsList[i];
		if(!plugin || !plugin->unload) continue;
		(*(pluginFunc)plugin->unload)();
	}
}
