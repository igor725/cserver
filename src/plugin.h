#ifndef PLUGIN_H
#define PLUGIN_H
#include "core.h"
#include "list.h"
#include "platform.h"

typedef cs_bool(*pluginInitFunc)(void);
typedef cs_bool(*pluginUnloadFunc)(cs_bool);
typedef void(*pluginReceiveIface)(cs_str name, void *ptr, cs_size size);
typedef struct _Plugin {
	cs_int8 id;
	cs_str name;
	cs_int32 version;
	void *lib;
	PluginInterface *ifaces;
	pluginReceiveIface irecv;
	pluginUnloadFunc unload;
	AListField *ireqHead;
	Mutex *mutex;
} Plugin;

#define Plugin_Lock(_p) Mutex_Lock((_p)->mutex)
#define Plugin_Unlock(_p) Mutex_Unlock((_p)->mutex)
void Plugin_LoadAll(void);
void Plugin_UnloadAll(cs_bool force);

API cs_bool Plugin_LoadDll(cs_str name);
API cs_bool Plugin_UnloadDll(Plugin *plugin, cs_bool force);
API Plugin *Plugin_Get(cs_str name);
API cs_bool Plugin_RequestInterface(pluginReceiveIface irecv, cs_str iname);
VAR Plugin *Plugins_List[MAX_PLUGINS];
#endif // PLUGIN_H
