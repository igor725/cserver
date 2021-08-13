#ifndef PLUGIN_H
#define PLUGIN_H
#include "core.h"

typedef cs_bool(*pluginFunc)(void);
typedef struct _Plugin {
	cs_int8 id;
	cs_str name;
	cs_int32 version;
	void *lib;
	pluginFunc unload;
} Plugin;

void Plugin_LoadAll(void);
void Plugin_UnloadAll(void);

API cs_bool Plugin_LoadDll(cs_str name);
API cs_bool Plugin_UnloadDll(Plugin *plugin);
API Plugin *Plugin_Get(cs_str name);
API cs_bool Plugin_GetSymbol(Plugin *plugin, cs_str name, void *proc);
VAR Plugin *Plugins_List[MAX_PLUGINS];
#endif // PLUGIN_H
