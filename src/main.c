#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"
#include "server.h"
#include "log.h"

cs_int32 main(cs_int32 argc, cs_char **argv) {
	Memory_Init();
	Log_Init();
	Http_Init();
	if(argc < 2 || !String_CaselessCompare(argv[1], "nochdir")) {
		cs_str path = String_AllocCopy(argv[0]);
		cs_char *lastSlash = (cs_char *)String_LastChar(path, *PATH_DELIM);
		if(lastSlash) {
			*lastSlash = '\0';
			Directory_SetCurrentDir(path);
		}
		Memory_Free((void *)path);
	}

	Server_InitialWork();
	Server_StartLoop();
	Server_Stop();
	Http_Uninit();
	Log_Uninit();
	Memory_Uninit();
	return 0;
}
