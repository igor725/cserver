#include <core.h>

#include "http.h"


EXP int Plugin_ApiVer = 100;
EXP bool Plugin_Init() {
	return Http_StartServer("0.0.0.0", 8080);
}
