#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"
#include "server.h"
#include "log.h"
#include "cserror.h"
#include "hash.h"
#include "compr.h"
#include "tests.h"

INL static cs_bool Init(void) {
	return Memory_Init() && Log_Init()
	&& Error_Init() && Socket_Init();
}

int main(int argc, char *argv[]) {
	if(Init()) {
		cs_bool testmode = false, nochdir = false;

		if(argc > 1) {
			for(cs_int32 i = 1; i < argc; i++) {
				if(String_CaselessCompare(argv[i], "testmode"))
					testmode = true;
				else if(String_CaselessCompare(argv[i], "nochdir"))
					nochdir = true;
			}
		}

		if(!nochdir) {
			cs_char *lastSlash = (cs_char *)String_LastChar(argv[0], *PATH_DELIM);
			if(lastSlash) {
				// Отсоединяем имя запускаемого файла и
				// тем самым получаем путь до него.
				*lastSlash = '\0';
				Directory_SetCurrentDir(argv[0]);
			}
		}

		if(testmode) {
			if(Tests_PerformAll())
				Log_Info("All tests passed!");
			else
				Log_Error("Some tests failed!");

			Process_Exit(1);
		} else {
			if(Server_Init())
				Server_StartLoop();
		}

		Server_Cleanup();
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
