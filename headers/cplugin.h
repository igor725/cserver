#ifndef CPLUGIN_H
#define CPLUGIN_H
typedef bool(*pluginFunc)(void);
typedef struct cPlugin {
	const char* name;
	int32_t id;
	void* lib;
	pluginFunc unload;
} *CPlugin;

void CPlugin_Start(void);
void CPlugin_Stop(void);
bool CPlugin_Load(const char* name);
bool CPlugin_Unload(CPlugin plugin);
CPlugin CPlugin_Get(const char* name);
CPlugin CPLugins_List[MAX_PLUGINS];
#endif
