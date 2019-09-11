#ifndef CPLUGIN_H
#define CPLUGIN_H

#define CPLUGIN_API_NUM 100

typedef void (*initFunc)();
typedef struct cPlugin {
	const char* name;
	void* handle;
	initFunc func;
	bool loaded;
	struct cPlugin* prev;
	struct cPlugin* next;
} CPLUGIN;

void CPlugin_Start();
void CPlugin_Stop();
#endif
