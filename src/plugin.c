#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "plugin.h"
#include "strstor.h"
#include "list.h"

Plugin *Plugins_List[MAX_PLUGINS] = {NULL};

INL static void AddInterface(Plugin *requester, PluginInterface *iface) {
	void *ptr = Memory_Alloc(1, iface->isize);
	Memory_Copy(ptr, iface->iptr, iface->isize);
	AList_AddField(&requester->ireqHead, (void *)iface->iname);
	requester->irecv(iface->iname, ptr, iface->isize);
}

INL static cs_bool CheckHoldIfaces(Plugin *plugin) {
	for(cs_int8 i = 0; i < MAX_PLUGINS; i++) {
		AListField *hold;
		Plugin *tplugin = Plugins_List[i];
		if(tplugin == plugin || !tplugin) continue;
		PluginInterface *iface, *tiface;
		for(iface = plugin->ifaces; iface && iface->iname; iface++) {
			for(tiface = tplugin->ifaces; tiface && tiface->iname; tiface++)
				if(String_Compare(iface->iname, tiface->iname)) return false;

			List_Iter(hold, tplugin->ireqHold) {
				if(String_Compare(iface->iname, hold->value.str)) {
					Memory_Free(hold->value.ptr);
					AList_Remove(&tplugin->ireqHold, hold);
					AddInterface(tplugin, iface);
				}
			}
		}
	}

	return true;
}

cs_bool Plugin_LoadDll(cs_str name) {
	if(!String_IsSafe(name)) return false;

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

		if(*apiVerSym != PLUGIN_API_NUM) {
			if(*apiVerSym < PLUGIN_API_NUM)
				Log_Error(Sstor_Get("PLUG_DEPR"), name, PLUGIN_API_NUM, *apiVerSym);
			else
				Log_Error(Sstor_Get("PLUG_DEPR_API"), name, *apiVerSym, PLUGIN_API_NUM);

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
			if(!Plugins_List[i] && plugin->id == -1) {
				plugin->id = i;
				break;
			}
		}

		if(plugin->id != -1) {
			if(!plugin->ifaces || CheckHoldIfaces(plugin)) {
				Plugins_List[plugin->id] = plugin;
				if(initSym()) return true;
				Log_Error(Sstor_Get("PLUG_ERROR"), path);
			} else
				Log_Error(Sstor_Get("PLUG_ITFS"), name);
		}

		Plugin_UnloadDll(plugin, true);
		return false;
	}

	Log_Error("%s: %s", path, DLib_GetError(error, 512));
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

	for(cs_int32 i = 0; i < MAX_PLUGINS && !(requester && answerer); i++) {
		Plugin *cplugin = Plugins_List[i];
		if(!cplugin) continue;

		if(!requester && irecv == cplugin->irecv && cplugin != answerer) {
			Plugin_Lock(cplugin);
			AListField *tmp;

			/**
			 * Проверяем, нету ли указанного интерфейса в активных
			 * интерфейсах плагина, если он там есть, то возвращаем true.
			 */
			List_Iter(tmp, cplugin->ireqHead) {
				if(String_Compare(tmp->value.ptr, iname)) {
					Plugin_Unlock(cplugin);
					if(answerer) Plugin_Unlock(answerer);
					return false;
				}
			}

			/**
			 * Проверяем, нету ли указанного интерфейса в холдлисте плагина,
			 * если он там есть, то возвращаем true.
			 * 
			 * P.S. Да, функция возвращает true если повторно запросить интерфейс,
			 * который ещё не успел уйти из холдлиста, но если он уже активен, то
			 * функция вернёт false. Вроде как такой финт выглядит правильно.
			 */
			List_Iter(tmp, cplugin->ireqHold) {
				if(String_Compare(tmp->value.ptr, iname)) {
					Plugin_Unlock(cplugin);
					if(answerer) Plugin_Unlock(answerer);
					return true;
				}
			}

			requester = cplugin;
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
		AddInterface(requester, iface);
		Plugin_Unlock(requester);
		Plugin_Unlock(answerer);
		return true;
	} else if(requester) {
		AList_AddField(&requester->ireqHold, (void *)String_AllocCopy(iname));
		Plugin_Unlock(requester);
		return true;
	}

	if(requester) Plugin_Unlock(requester);
	if(answerer) Plugin_Unlock(answerer);
	return false;
}

INL static cs_bool TryDiscard(Plugin *plugin, AListField **head, cs_str iname) {
	Plugin_Lock(plugin);

	AListField *tmp;
	List_Iter(tmp, *head) {
		if(String_Compare(tmp->value.str, iname)) {
			/**
			 * Так как в списке холда все названия интерфейсов
			 * хранятся в виде копии оригинальной строки, дабы
			 * не словить утечку памяти, мы очищаем её здесь.
			 * 
			 * P.S. Если интерфейс был в списке холда, то
			 * коллбек с установкой поинтера в ноль не вызывается,
			 * в этом мало смысла.
			 * 
			 */
			if(*head == plugin->ireqHold)
				Memory_Free(tmp->value.ptr);
			else
				plugin->irecv(iname, NULL, 0);
			AList_Remove(head, tmp);
			Plugin_Unlock(plugin);
			return true;
		}
	}

	Plugin_Unlock(plugin);
	return false;
}

cs_bool Plugin_DiscardInterface(pluginReceiveIface irecv, cs_str iname) {
	for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
		Plugin *cplugin = Plugins_List[i];
		if(cplugin && cplugin->irecv == irecv) {
			if(TryDiscard(cplugin, &cplugin->ireqHead, iname)) return true;
			if(TryDiscard(cplugin, &cplugin->ireqHold, iname)) return true;
			break;
		}
	}

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
						AList_AddField(&tplugin->ireqHold, (void *)String_AllocCopy(iface->iname));
						tplugin->irecv(iface->iname, NULL, 0);
						break;
					}
				}
				Plugin_Unlock(tplugin);
			}
		}
	}

	while(plugin->ireqHold) {
		Memory_Free(plugin->ireqHold->value.ptr);
		AList_Remove(&plugin->ireqHold, plugin->ireqHold);
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
	Directory_Ensure("plugins" PATH_DELIM "disabled");

	DirIter pIter = {0};
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
