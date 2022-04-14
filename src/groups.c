#include "core.h"
#include "str.h"
#include "list.h"
#include "platform.h"
#include "groups.h"

static KListField *headCGroup = NULL;

cs_uintptr Groups_Create(cs_str name, cs_byte rank) {
	KListField *gptr;
	CGroup *tmpgrp;
	List_Iter(gptr, headCGroup) {
		tmpgrp = (CGroup *)gptr->value.ptr;
		if(String_CaselessCompare(tmpgrp->name, name))
			return GROUPS_INVALID_ID;
	}

	cs_uintptr nextid = 0;
	if(headCGroup)
		nextid = headCGroup->key.numptr + 1;

	tmpgrp = Memory_Alloc(1, sizeof(CGroup));
	String_Copy(tmpgrp->name, 65, name);
	tmpgrp->rank = rank;
	KList_AddField(&headCGroup, (void *)nextid, tmpgrp);
	return nextid;
}

CGroup *Groups_GetByID(cs_uintptr gid) {
	KListField *gptr = NULL;

	List_Iter(gptr, headCGroup) {
		if(gptr->key.numptr == gid)
			return (CGroup *)gptr->value.ptr;
	}

	return NULL;
}

cs_bool Groups_Remove(cs_uintptr gid) {
	KListField *gptr = NULL;

	List_Iter(gptr, headCGroup) {
		if(gptr->key.numptr == gid) {
			Memory_Free(gptr->value.ptr);
			KList_Remove(&headCGroup, gptr);
			return true;
		}
	}

	return false;
}
