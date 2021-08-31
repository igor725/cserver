#ifndef STRSTOR_H
#define STRSTOR_H
#include "core.h"

cs_bool Sstor_Defaults(void);
API cs_bool Sstor_IsExists(cs_str key);
API cs_str Sstor_Get(cs_str key);
API cs_bool Sstor_Set(cs_str key, cs_str value);
API void Sstor_Cleanup(void);
#endif
