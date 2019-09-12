#ifdef CP_ENABLED
#include "core.h"
#include "event.h"
#include "client.h"
#include "server.h"
#include "cplugin.h"

bool CPlugin_Load(const char* name) {
	char path[256];
	String_FormatBuf(path, 256, "plugins/%s", name);
	void* plugin;
	void* verSym;
	void* initSym;

	if(DLib_Load(path, &plugin)) {
		if(!(DLib_GetSym(plugin, "Plugin_ApiVer", &verSym) &&
		DLib_GetSym(plugin, "Plugin_Init", &initSym)))
			return false;

		if(*((int*)verSym) / 100 != CPLUGIN_API_NUM / 100) {
			Error_Set(ET_SERVER, EC_DLLPLUGVER, true);
			return false;
		}
		return (*(initFunc)initSym)();
	}
	return false;
}

void CPlugin_Start() {
	Directory_Ensure("plugins");
	dirIter pIter = {0};
	if(Iter_Init(&pIter, "plugins", DLIB_EXT)) {
		do {
			if(pIter.isDir || !pIter.cfile) continue;
			if(!CPlugin_Load(pIter.cfile))
				Log_FormattedError();
		} while(Iter_Next(&pIter));
	}
}

void CPlugin_Stop() {

}
#endif
