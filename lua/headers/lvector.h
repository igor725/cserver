#ifndef LVECTOR_H
#define LVECTOR_H
#define VT_SHORT 0
#define VT_FLOAT 1

typedef struct _LuaVec {
	cs_int8 type;
	cs_bool allocated;
	union {
		SVec* s;
		Vec* f;
		void* p;
	} val;
} LuaVec;

LuaVec* luax_checkvec(lua_State* L, cs_int32 idx);
SVec* luax_checksvec(lua_State* L, cs_int32 idx);
Vec* luax_checkfvec(lua_State* L, cs_int32 idx);
void luax_pushvec(lua_State* L, LuaVec* vec);

LuaVec* luax_newpsvec(lua_State* L, SVec* vec);
LuaVec* luax_newpfvec(lua_State* L, Vec* vec);
#endif
