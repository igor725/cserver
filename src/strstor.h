#ifndef STRSTOR_H
#define STRSTOR_H
#include "core.h"

#ifndef CORE_BUILD_PLUGIN
	cs_bool Sstor_Defaults(void);
#endif

API cs_bool Sstor_IsExists(cs_str key);
API cs_str Sstor_Get(cs_str key);
API cs_bool Sstor_Set(cs_str key, cs_str value);
API void Sstor_Cleanup(void);
#endif
