#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"
#include "server.h"
#include "log.h"
#include "error.h"

cs_int32 main(cs_int32 argc, cs_char **argv) {
	if(Memory_Init() && Log_Init() && Http_Init()) {
		if(argc < 2 || !String_CaselessCompare(argv[1], "nochdir")) {
			cs_char *lastSlash = (cs_char *)String_LastChar(argv[0], *PATH_DELIM);
			if(lastSlash) {
				// Отсоединяем имя запускаемого файла и
				// тем самым получаем путь до него.
				*lastSlash = '\0';
				Directory_SetCurrentDir(argv[0]);
			}
		}

		if(Server_Init()) {
			Server_StartLoop();
			Server_Cleanup();
		}

		Http_Uninit();
		Log_Uninit();
		Memory_Uninit();
		return 0;
	}

	return Error_GetSysCode();
}
