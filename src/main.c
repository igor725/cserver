#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"
#include "server.h"
#include "log.h"
#include "cserror.h"
#include "hash.h"
#include "compr.h"
// #include "tests.h"

INL static cs_bool Init(void) {
	return Memory_Init() && Log_Init()
	&& Error_Init() && Socket_Init();
}

int main(int argc, char *argv[]) {
	if(Init()) {
		if(argc < 2 || !String_CaselessCompare(argv[1], "nochdir")) {
			cs_char *lastSlash = (cs_char *)String_LastChar(argv[0], *PATH_DELIM);
			if(lastSlash) {
				// Отсоединяем имя запускаемого файла и
				// тем самым получаем путь до него.
				*lastSlash = '\0';
				Directory_SetCurrentDir(argv[0]);
			}
		}

#ifndef CORE_TEST_MODE
		if(Server_Init()) {
			Server_StartLoop();
			Server_Cleanup();
		}
#else
		if(Tests_PerformAll())
			Log_Info("All tests passed!");
		else
			Log_Error("Some tests failed!");
#endif

		Compr_Uninit();
		Http_Uninit();
		Hash_Uninit();
		Socket_Uninit();
		Error_Uninit();
		Log_Uninit();
		Memory_Uninit();
		return 0;
	}

	return Error_GetSysCode();
}
