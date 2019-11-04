#ifndef PLUGIN_H
#define PLUGIN_H
typedef cs_bool(*pluginFunc)(void);
typedef struct cPlugin {
	const char* name;
	cs_int32 id;
	void* lib;
	pluginFunc unload;
} *Plugin;

void Plugin_Start(void);
void Plugin_Stop(void);
cs_bool Plugin_Load(const char* name);
cs_bool Plugin_Unload(Plugin plugin);
Plugin Plugin_Get(const char* name);
Plugin Plugins_List[MAX_PLUGINS];
#endif
