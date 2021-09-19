#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"
#include "server.h"
#include "log.h"
#include "cserror.h"
#include "hash.h"

INL static cs_bool Init(void) {
	return Memory_Init() &&
	Log_Init() && Error_Init() &&
	Socket_Init();
}

INL static void Uninit(void) {
	Hash_Uninit();
	Socket_Uninit();
	Error_Uninit();
	Log_Uninit();
	Memory_Uninit();
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

		if(Server_Init()) {
			Server_StartLoop();
			Server_Cleanup();
		}

		Uninit();
		return 0;
	}

	return Error_GetSysCode();
}
