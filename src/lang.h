#ifndef LANG_H
#define LANG_H
typedef struct _LGroup {
	cs_uint32 size;
	cs_str *strings;
} LGroup;

VAR LGroup *Lang_SwGrp, *Lang_ErrGrp, *Lang_ConGrp,
*Lang_KickGrp, *Lang_CmdGrp, *Lang_DbgGrp;

void Lang_Init(void);

API LGroup *Lang_NewGroup(cs_uint32 size);
API cs_uint32 Lang_ResizeGroup(LGroup *grp, cs_uint32 newsize);
API cs_bool Lang_Set(LGroup *grp, cs_uint32 id, cs_str str);
API cs_str Lang_Get(LGroup *grp, cs_uint32 id);
#endif // LANG_H
