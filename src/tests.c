#include "core.h"
#include "log.h"
#include "tests.h"

#include "tests/memory.c"
#include "tests/strings.c"
#include "tests/client.c"
#include "tests/world.c"
#include "tests/config.c"

cs_uint16 Tests_CurrNum = 0;
cs_str Tests_Current = NULL;
void Tests_NewTask(cs_str name) {
	Log_Info("Test %d: %s", ++Tests_CurrNum, name);
	Tests_Current = name;
}

cs_bool Tests_PerformAll(void) {
	return Tests_Memory() &&
	Tests_Strings() &&
	Tests_Client() &&
	Tests_World() &&
	Tests_Config();
}
