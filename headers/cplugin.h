#ifndef CPLUGIN_H
#define CPLUGIN_H

#define CPLUGIN_API_NUM 100

typedef bool (*initFunc)();
bool CPlugin_Load(const char* name);
void CPlugin_Start();
#endif
