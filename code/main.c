#include "core.h"
#include "platform.h"
#include "str.h"
#include "server.h"

int32_t main(int32_t argc, char** argv) {
	if(argc < 2 || !String_CaselessCompare(argv[1], "nochdir")) {
		const char* path = String_AllocCopy(argv[0]);
		char* lastSlash = (char*)String_LastChar(path, PATH_DELIM);
		if(lastSlash) {
			*lastSlash = '\0';
			Directory_SetCurrentDir(path);
		}
		Memory_Free((char*)path);
	}

	Server_InitialWork();
	Server_StartLoop();
	Server_Stop();
	return 0;
}
