#ifndef PLUGIN_H
#define PLUGIN_H
#include "core.h"

typedef cs_bool(*pluginInitFunc)(void);
typedef cs_bool(*pluginUnloadFunc)(cs_bool);
typedef struct _Plugin {
	cs_int8 id;
	cs_str name;
	cs_int32 version;
	void *lib;
	pluginUnloadFunc unload;
} Plugin;

void Plugin_LoadAll(void);
void Plugin_UnloadAll(cs_bool force);

API cs_bool Plugin_LoadDll(cs_str name);
API cs_bool Plugin_UnloadDll(Plugin *plugin, cs_bool force);
API Plugin *Plugin_Get(cs_str name);
API cs_bool Plugin_GetSymbol(Plugin *plugin, cs_str name, void *proc);
VAR Plugin *Plugins_List[MAX_PLUGINS];
#endif // PLUGIN_H
