#ifndef GROUPSTYPES_H
#define GROUPSTYPES_H
#include "core.h"

#define GROUPS_INVALID_ID (cs_uintptr)-1

typedef struct _CGroup {
	cs_byte rank;
	cs_char name[65];
} CGroup;
#endif
