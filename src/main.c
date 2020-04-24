#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"
#include "server.h"

cs_int32 main(cs_int32 argc, cs_char **argv) {
	Memory_Init();
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
	Memory_Uninit();
	Http_Uninit();
	return 0;
}
