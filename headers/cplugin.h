#ifndef CPLUGIN_H
#define CPLUGIN_H
typedef bool(*pluginFunc)(void);
typedef struct cPlugin {
	const char* name;
	int32_t id;
	void* lib;
	pluginFunc unload;
} *CPLUGIN;

void CPlugin_Start(void);
void CPlugin_Stop(void);
bool CPlugin_Load(const char* name);
bool CPlugin_Unload(CPLUGIN plugin);
CPLUGIN CPlugin_Get(const char* name);
CPLUGIN CPLugins_List[MAX_PLUGINS];
#endif
