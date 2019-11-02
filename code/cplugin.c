#include "core.h"
#include "platform.h"
#include "str.h"
#include "cplugin.h"
#include "command.h"
#include "lang.h"

CPlugin CPLugins_List[MAX_PLUGINS] = {0};

bool CPlugin_Load(const char* name) {
	char path[256];
	char error[512];
	String_FormatBuf(path, 256, "plugins/%s", name);
	if(CPlugin_Get(name)) return false;
	void *lib, *verSym, *initSym;

	if(DLib_Load(path, &lib)) {
		if(!(DLib_GetSym(lib, "Plugin_ApiVer", &verSym) &&
		DLib_GetSym(lib, "Plugin_Load", &initSym))) {
			Log_Error("%s: %s", path, DLib_GetError(error, 512));
			DLib_Unload(lib);
			return false;
		}

		int32_t ver = *((int32_t*)verSym);
		if(ver != CPLUGIN_API_NUM) {
			if(ver < CPLUGIN_API_NUM)
				Log_Error(Lang_Get(LANG_CPAPIOLD), name, CPLUGIN_API_NUM, ver);
			else
				Log_Error(Lang_Get(LANG_CPAPIUPG), name, ver, CPLUGIN_API_NUM);

			DLib_Unload(lib);
			return false;
		}

		CPlugin plugin = Memory_Alloc(1, sizeof(struct cPlugin));
		DLib_GetSym(lib, "Plugin_Unload", (void*)&plugin->unload);

		plugin->name = String_AllocCopy(name);
		plugin->lib = lib;
		plugin->id = -1;

		for(int32_t i = 0; i < MAX_PLUGINS; i++) {
			if(!CPLugins_List[i]) {
				CPLugins_List[i] = plugin;
				plugin->id = i;
				break;
			}
		}

		if(plugin->id == -1 || !(*(pluginFunc)initSym)()) {
			CPlugin_Unload(plugin);
			return false;
		}

		return true;
	}

	Log_Error("%s: %s", path, DLib_GetError(error, 512));
	return false;
}

CPlugin CPlugin_Get(const char* name) {
	for(int32_t i = 0; i < MAX_PLUGINS; i++) {
		CPlugin ptr = CPLugins_List[i];
		if(ptr && String_Compare(ptr->name, name)) return ptr;
	}
	return NULL;
}

bool CPlugin_Unload(CPlugin plugin) {
	if(plugin->unload && !(*(pluginFunc)plugin->unload)())
		return false;
	if(plugin->name)
		Memory_Free((void*)plugin->name);
	if(plugin->id != -1)
		CPLugins_List[plugin->id] = NULL;

	DLib_Unload(plugin->lib);
	Memory_Free(plugin);
	return true;
}

void CPlugin_Start(void) {
	Directory_Ensure("plugins");

	dirIter pIter = {0};
	if(Iter_Init(&pIter, "plugins", DLIB_EXT)) {
		do {
			if(!pIter.isDir && pIter.cfile)
				CPlugin_Load(pIter.cfile);
		} while(Iter_Next(&pIter));
	}
}

void CPlugin_Stop(void) {
	for(int32_t i = 0; i < MAX_PLUGINS; i++) {
		CPlugin plugin = CPLugins_List[i];
		if(plugin && plugin->unload)
			(*(pluginFunc)plugin->unload)();
	}
}
