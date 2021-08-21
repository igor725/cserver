#ifndef LANG_H
#define LANG_H
#include "core.h"

typedef struct _LGroup {
	cs_uint32 size;
	cs_str *strings;
} LGroup;

VAR LGroup *Lang_SwGrp, *Lang_ErrGrp, *Lang_ConGrp,
*Lang_KickGrp, *Lang_CmdGrp;

cs_bool Lang_Init(void);
void Lang_Uninit(void);

API LGroup *Lang_NewGroup(cs_uint32 size);
API void Lang_FreeGroup(LGroup *grp);
API cs_uint32 Lang_ResizeGroup(LGroup *grp, cs_uint32 newsize);
API cs_bool Lang_Set(LGroup *grp, cs_uint32 id, cs_str str);
API cs_str Lang_Get(LGroup *grp, cs_uint32 id);
#endif // LANG_H
