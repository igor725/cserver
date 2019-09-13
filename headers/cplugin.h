#ifndef CPLUGIN_H
#define CPLUGIN_H

#define CPLUGIN_API_NUM 100
#define CPLUGIN_OLDMSG "Plugin %s is too old. Server uses PluginAPI v%03d, but plugin compiled for v%03d."
#define CPLUGIN_UPGMSG "Please upgrade your server software. Plugin %s compiled for PluginAPI v%03d, but server uses v%d."

typedef bool (*initFunc)();
bool CPlugin_Load(const char* name);
void CPlugin_Start();
#endif
