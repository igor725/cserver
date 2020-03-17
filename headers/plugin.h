#ifndef PLUGIN_H
#define PLUGIN_H
typedef cs_bool(*pluginFunc)(void);
typedef struct {
	cs_int8 id;
	cs_str name;
	cs_int32 version;
	void *lib;
	pluginFunc unload;
} Plugin;

void Plugin_Start(void);
void Plugin_Stop(void);
cs_bool Plugin_Load(cs_str name);
cs_bool Plugin_Unload(Plugin *plugin);
Plugin *Plugin_Get(cs_str name);
Plugin *Plugins_List[MAX_PLUGINS];
#endif // PLUGIN_H
