#include "core.h"
#include "event.h"
#include "client.h"
#include "server.h"
#include "cplugin.h"
#include "command.h"

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
			if(!CPlugin_List[i]) {
				CPlugin_List[i] = splugin;
				pluginId = i;
				break;
			}
		}
		splugin->id = pluginId;
		if(pluginId == -1 || !(*(pluginFunc)initSym)()) {
			CPlugin_Unload(splugin);
			return false;
		}

		return true;
	}

	Log_Error("%s: %s", path, DLib_GetError(error, 512));
	return false;
}

CPLUGIN* CPlugin_Get(const char* name) {
	for(int i = 0; i < MAX_PLUGINS; i++) {
		CPLUGIN* ptr = CPlugin_List[i];
		if(!ptr) continue;
		if(String_Compare(ptr->name, name)) return ptr;
	}
	return NULL;
}

bool CPlugin_Unload(CPLUGIN* plugin) {
	if(plugin->unload && !(*(pluginFunc)plugin->unload)())
		return false;

	if(plugin->name)
		Memory_Free((void*)plugin->name);
	if(plugin->id != -1)
		CPlugin_List[plugin->id] = NULL;

	DLib_Unload(plugin->lib);
	Memory_Free(plugin);
	return true;
}

#define GetPluginName \
if(!String_GetArgument(args, name, 64, 1)) { \
	String_Copy(out, CMD_MAX_OUT, "Invalid plugin name"); \
	return true; \
}

static bool CHandler_Plugins(const char* args, CLIENT* caller, char* out) {
	char command[64];
	char name[64];
	CPLUGIN* plugin;

	if(String_GetArgument(args, command, 64, 0)) {
		if(String_CaselessCompare(command, "load")) {
			GetPluginName;
			if(!CPlugin_Get(name) && CPlugin_Load(name))
				String_FormatBuf(out, CMD_MAX_OUT, "Plugin %s successfully loaded", name);
			else
				String_FormatBuf(out, CMD_MAX_OUT, "Plugin %s can't be loaded", name);

		} else if(String_CaselessCompare(command, "unload")) {
			GetPluginName;
			plugin = CPlugin_Get(name);
			if(!plugin) {
				String_Copy(out, CMD_MAX_OUT, "This plugin is not loaded");
				return true;
			}
			if(CPlugin_Unload(plugin))
				String_FormatBuf(out, CMD_MAX_OUT, "Plugin %s successfully unloaded", name);
			else
				String_FormatBuf(out, CMD_MAX_OUT, "Plugin %s can't be unloaded", name);
		} else if(String_CaselessCompare(command, "list")) {
			Log_Info("Loaded plugins list:");
			for(int i = 0; i < MAX_PLUGINS; i++) {
				CPLUGIN* plugin = CPlugin_List[i];
				if(!plugin) continue;
				Log_Info(plugin->name);
			}
			return false;
		} else {
			String_Copy(out, CMD_MAX_OUT, "Unknown plugins command");
		}
	} else {
		String_Copy(out, CMD_MAX_OUT, "Usage: plugin <command> [pluginName]");
	}

	return true;
}

void CPlugin_Start() {
	Command_Register("plugins", CHandler_Plugins);
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
		CPLUGIN* plugin = CPlugin_List[i];
		if(!plugin || !plugin->unload) continue;
		(*(pluginFunc)plugin->unload)();
	}
}
