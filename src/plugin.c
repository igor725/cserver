#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "plugin.h"
#include "strstor.h"

Plugin *Plugins_List[MAX_PLUGINS] = {0};

cs_bool Plugin_LoadDll(cs_str name) {
	cs_char path[256], error[512];
	void *lib;
	pluginInitFunc initSym;
	cs_int32 *apiVerSym;
	cs_int32 *plugVerSym;
	String_FormatBuf(path, 256, "plugins" PATH_DELIM "%s", name);

	if(DLib_Load(path, &lib)) {
		if(!(DLib_GetSym(lib, "Plugin_ApiVer", (void *)&apiVerSym) &&
		DLib_GetSym(lib, "Plugin_Load", (void *)&initSym))) {
			Log_Error("%s: %s", path, DLib_GetError(error, 512));
			DLib_Unload(lib);
			return false;
		}

		cs_int32 apiVer = *apiVerSym;
		if(apiVer != PLUGIN_API_NUM) {
			if(apiVer < PLUGIN_API_NUM)
				Log_Error(Sstor_Get("PLUG_DEPR"), name, PLUGIN_API_NUM, apiVer);
			else
				Log_Error(Sstor_Get("PLUG_DEPR_API"), name, apiVer, PLUGIN_API_NUM);

			DLib_Unload(lib);
			return false;
		}

		Plugin *plugin = Memory_Alloc(1, sizeof(Plugin));
		DLib_GetSym(lib, "Plugin_Unload", (void *)&plugin->unload);
		DLib_GetSym(lib, "Plugin_Interfaces", (void *)&plugin->ifaces);
		DLib_GetSym(lib, "Plugin_RecvInterface", (void *)&plugin->irecv);

		plugin->mutex = Mutex_Create();
		plugin->name = String_AllocCopy(name);
		if(DLib_GetSym(lib, "Plugin_Version", (void *)&plugVerSym))
			plugin->version = *plugVerSym;
		plugin->lib = lib;
		plugin->id = -1;

		for(cs_int8 i = 0; i < MAX_PLUGINS; i++) {
			if(!Plugins_List[i]) {
				plugin->id = i;
				break;
			}
		}

		if(plugin->id == -1 || !initSym()) {
			Plugin_UnloadDll(plugin, true);
			return false;
		}

		Plugins_List[plugin->id] = plugin;
		return true;
	}

	Log_Error(Sstor_Get("PLUG_LIBERR"), path, DLib_GetError(error, 512));
	return false;
}

Plugin *Plugin_Get(cs_str name) {
	for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
		Plugin *ptr = Plugins_List[i];
		if(ptr && String_Compare(ptr->name, name)) return ptr;
	}

	return NULL;
}

cs_bool Plugin_RequestInterface(pluginReceiveIface irecv, cs_str iname) {
	if(!irecv || !iname) return false;

	PluginInterface *iface = NULL;
	Plugin *requester = NULL,
	*answerer = NULL;

	for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
		Plugin *cplugin = Plugins_List[i];
		if(!cplugin) continue;

		if(!requester && irecv == cplugin->irecv && cplugin != answerer) {
			requester = cplugin;
			AListField *tmp;
			Plugin_Lock(requester);
			List_Iter(tmp, requester->ireqHead) {
				if(String_Compare(tmp->value.ptr, iname)) {
					Plugin_Unlock(requester);
					if(answerer) Plugin_Unlock(answerer);
					return false;
				}
			}

			continue;
		}
		
		if(!answerer && cplugin != requester) {
			Plugin_Lock(cplugin);
			for(iface = cplugin->ifaces; iface && iface->iname; iface++) {
				if(String_Compare(iface->iname, iname)) {
					answerer = cplugin;
					break;
				}
			}
			if(!answerer) Plugin_Unlock(cplugin);
		}
	}

	if(requester && answerer) {
		void *ptr = Memory_Alloc(1, iface->isize);
		Memory_Copy(ptr, iface->iptr, iface->isize);
		AList_AddField(&requester->ireqHead, (void *)iface->iname);
		requester->irecv(iface->iname, ptr, iface->isize);
		Plugin_Unlock(requester);
		Plugin_Unlock(answerer);
		return true;
	}

	if(requester) Plugin_Unlock(requester);
	if(answerer) Plugin_Unlock(answerer);
	return false;
}

cs_bool Plugin_UnloadDll(Plugin *plugin, cs_bool force) {
	Plugin_Lock(plugin);
	if(plugin->unload && !(*(pluginUnloadFunc)plugin->unload)(force) && !force) {
		Plugin_Unlock(plugin);
		return false;
	}

	if(plugin->name) {
		Memory_Free((void *)plugin->name);
		plugin->name = NULL;
	}

	if(plugin->ifaces) {
		for(PluginInterface *iface = plugin->ifaces; iface->iname; iface++) {
			for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
				Plugin *tplugin = Plugins_List[i];
				if(tplugin == plugin || !tplugin) continue;
				Plugin_Lock(tplugin);
				AListField *tmp;
				List_Iter(tmp, tplugin->ireqHead) {
					if(tmp->value.ptr == iface->iname) {
						AList_Remove(&tplugin->ireqHead, tmp);
						tplugin->irecv(iface->iname, NULL, 0);
						break;
					}
				}
				Plugin_Unlock(tplugin);
			}
		}
	}

	while(plugin->ireqHead)
		AList_Remove(&plugin->ireqHead, plugin->ireqHead);

	if(plugin->id != -1) {
		Plugins_List[plugin->id] = NULL;
		plugin->id = -1;
	}
	Plugin_Unlock(plugin);

	Mutex_Free(plugin->mutex);
	DLib_Unload(plugin->lib);
	Memory_Free(plugin);
	return true;
}

void Plugin_LoadAll(void) {
	Directory_Ensure("plugins");

	DirIter pIter;
	if(Iter_Init(&pIter, "plugins", DLIB_EXT)) {
		do {
			if(!pIter.isDir && pIter.cfile)
				Plugin_LoadDll(pIter.cfile);
		} while(Iter_Next(&pIter));
	}
	Iter_Close(&pIter);
}

void Plugin_UnloadAll(cs_bool force) {
	for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
		Plugin *plugin = Plugins_List[i];
		if(plugin) Plugin_UnloadDll(plugin, force);
	}
}
