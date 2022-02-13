#ifndef PLUGIN_H
#define PLUGIN_H
#include "core.h"
#include "list.h"
#include "platform.h"

typedef struct _PluginInterface {
	cs_str iname;
	void *iptr;
	cs_size isize;
} PluginInterface;

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

#ifndef CORE_BUILD_PLUGIN
void Plugin_LoadAll(void);
void Plugin_UnloadAll(cs_bool force);
#else
EXP cs_bool Plugin_Load(void);
EXP cs_bool Plugin_Unload(cs_bool force);
EXP void Plugin_RecvInterface(cs_str name, void *ptr, cs_size size);
EXP cs_int32 Plugin_ApiVer, Plugin_Version;
#define Plugin_DeclareInterfaces(_N) EXP PluginInterface Plugin_Interfaces[_N]; \
PluginInterface Plugin_Interfaces[_N] = 
#define Plugin_SetVersion(ver) cs_int32 Plugin_ApiVer = PLUGIN_API_NUM, Plugin_Version = ver;
#define PLUGIN_IFACE_END {NULL, NULL, 0}
#define PLUGIN_IFACE_ADD(n, i) {n, &(i), sizeof(i)},
#endif
API cs_bool Plugin_LoadDll(cs_str name);
API cs_bool Plugin_UnloadDll(Plugin *plugin, cs_bool force);
API Plugin *Plugin_Get(cs_str name);
API cs_bool Plugin_RequestInterface(pluginReceiveIface irecv, cs_str iname);
VAR Plugin *Plugins_List[MAX_PLUGINS];
#endif // PLUGIN_H
