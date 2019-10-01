#ifndef CPLUGIN_H
#define CPLUGIN_H

#define MAX_PLUGINS     32
#define CPLUGIN_API_NUM 100
#define CPLUGIN_OLDMSG "Plugin %s is too old. Server uses PluginAPI v%03d, but plugin compiled for v%03d."
#define CPLUGIN_UPGMSG "Please upgrade your server software. Plugin %s compiled for PluginAPI v%03d, but server uses v%d."

typedef bool (*pluginFunc)();
typedef struct cPlugin {
	const char* name;
	uint32_t id;
	void* lib;
	pluginFunc unload;
} *CPLUGIN;

CPLUGIN CPlugin_List[MAX_PLUGINS];

API bool CPlugin_Load(const char* name);
API bool CPlugin_Unload(CPLUGIN plugin);
API CPLUGIN CPlugin_Get(const char* name);

void CPlugin_Start();
void CPlugin_Stop();
#endif
