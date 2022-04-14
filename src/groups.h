#ifndef GROUPS_H
#define GROUPS_H
#include "core.h"
#include "types/groups.h"

API cs_uintptr Groups_Create(cs_str name, cs_byte rank);
API CGroup *Groups_GetByID(cs_uintptr gid);
API cs_bool Groups_Remove(cs_uintptr gid);
#endif
