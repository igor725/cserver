#ifndef PLUGINTYPES_H
#define PLUGINTYPES_H
#include "core.h"
#include "types/platform.h"
#include "types/list.h"

typedef struct _PluginInterface {
	cs_str iname;
	void *iptr;
	cs_size isize;
} PluginInterface;

typedef struct _PluginInfo {
	cs_uint32 id, version;
	cs_str name, home;
} PluginInfo;

typedef cs_bool(*pluginInitFunc)(void);
typedef cs_bool(*pluginInitExFunc)(cs_uint32 id);
typedef cs_bool(*pluginUnloadFunc)(cs_bool);
typedef void(*pluginReceiveIface)(cs_str name, void *ptr, cs_size size);
typedef cs_str(*pluginUrlFunc)(void);

typedef struct _Plugin {
	cs_uint32 id, version;
	cs_str name;
	void *lib;
	PluginInterface *ifaces;
	pluginReceiveIface irecv;
	pluginUnloadFunc unload;
	pluginUrlFunc url;
	AListField *ireqHead;
	AListField *ireqHold;
	Mutex *mutex;
} Plugin;
#endif
