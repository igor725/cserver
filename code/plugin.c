#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "plugin.h"
#include "command.h"
#include "lang.h"

Plugin* pluginsList[MAX_PLUGINS];

cs_bool Plugin_Load(const char* name) {
	char path[256], error[512];
	void *lib;
	pluginFunc initSym;
	cs_int32* apiVerSym;
	cs_int32* plugVerSym;
	String_FormatBuf(path, 256, "plugins" PATH_DELIM "%s", name);

	if(DLib_Load(path, &lib)) {
		if(!(DLib_GetSym(lib, "Plugin_ApiVer", (void*)&apiVerSym) &&
		DLib_GetSym(lib, "Plugin_Load", (void*)&initSym))) {
			Log_Error("%s: %s", path, DLib_GetError(error, 512));
			DLib_Unload(lib);
			return false;
		}

		DLib_GetSym(lib, "Plugin_Version", (void*)&plugVerSym);
		cs_int32 apiVer = *apiVerSym;
		if(apiVer != PLUGIN_API_NUM) {
			if(apiVer < PLUGIN_API_NUM)
				Log_Error(Lang_Get(LANG_CPAPIOLD), name, PLUGIN_API_NUM, apiVer);
			else
				Log_Error(Lang_Get(LANG_CPAPIUPG), name, apiVer, PLUGIN_API_NUM);

			DLib_Unload(lib);
			return false;
		}

		Plugin* plugin = Memory_Alloc(1, sizeof(Plugin));
		DLib_GetSym(lib, "Plugin_Unload", (void*)&plugin->unload);

		plugin->name = String_AllocCopy(name);
		if(plugVerSym) plugin->version = *plugVerSym;
		plugin->lib = lib;
		plugin->id = -1;

		for(cs_int8 i = 0; i < MAX_PLUGINS; i++) {
			if(!pluginsList[i]) {
				pluginsList[i] = plugin;
				plugin->id = i;
				break;
			}
		}

		if(plugin->id == -1 || !initSym()) {
			Plugin_Unload(plugin);
			return false;
		}

		return true;
	}

	Log_Error("%s: %s", path, DLib_GetError(error, 512));
	return false;
}

Plugin* Plugin_Get(const char* name) {
	for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
		Plugin* ptr = pluginsList[i];
		if(ptr && String_Compare(ptr->name, name)) return ptr;
	}
	return NULL;
}

cs_bool Plugin_Unload(Plugin* plugin) {
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

void Plugin_Start(void) {
	Directory_Ensure("plugins");

	dirIter pIter;
	if(Iter_Init(&pIter, "plugins", DLIB_EXT)) {
		do {
			if(!pIter.isDir && pIter.cfile)
				Plugin_Load(pIter.cfile);
		} while(Iter_Next(&pIter));
	}
	Iter_Close(&pIter);
}

void Plugin_Stop(void) {
	for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
		Plugin* plugin = pluginsList[i];
		if(plugin && plugin->unload)
			(*(pluginFunc)plugin->unload)();
	}
}
