#ifdef CORE_BUILD_PLUGIN
#	error Tests cannot be used in plugins
#endif

#ifndef TESTS_H
#define TESTS_H
#include "core.h"
#include "log.h"

extern cs_uint16 Tests_CurrNum;
extern cs_str Tests_Current;
void Tests_NewTask(cs_str name);
#define Tests_Assert(expr, desc) if(!(expr)) { \
	Log_Error("\tFailed: %s.", desc); \
	return false; \
}
cs_bool Tests_PerformAll(void);
#endif
