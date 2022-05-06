#ifndef PAGER_H
#define PAGER_H
#include "core.h"

typedef struct _Pager {
	cs_int32 state;
	cs_int32 plen;
	cs_int32 pcur;
} Pager;

#define PAGER_DEFAULT_PAGELEN 10

#define Pager_Init(c, p) (Pager){.state = (c < 1 ? 0 : (c - 1)) * (p), .plen = p, .pcur = c < 1 ? 1 : c}
#define Pager_Step(s) if((s).state-- > 0) continue; else if((s).state < -(s).plen) continue;
#define Pager_IsDirty(s) (((s).state < -(s).plen) || ((s).pcur != 1))
#define Pager_CurrentPage(s) ((s).pcur)
#define Pager_CountPages(s) (((s).plen * ((s).pcur - 1) -(s).state) / (s).plen)
#endif
