#ifndef CPLUGIN_H
#define CPLUGIN_H

#define CPLUGIN_API_NUM 100

typedef void (*initFunc)();
void CPlugin_Start();
void CPlugin_Stop();
#endif
