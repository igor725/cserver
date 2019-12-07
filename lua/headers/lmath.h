#ifndef LMATH_H
#define LMATH_H
#define VT_SHORT 0
#define VT_FLOAT 1

typedef struct _LuaVec {
	cs_int8 type;
	cs_bool allocated;
	union {
		SVec* s;
		Vec* f;
		void* p;
	} value;
} LuaVec;

typedef struct _LuaAng {
	cs_bool allocated;
	Ang* value;
} LuaAng;

LuaVec* luax_checkvec(lua_State* L, cs_int32 idx);
SVec* luax_checksvec(lua_State* L, cs_int32 idx);
Vec* luax_checkfvec(lua_State* L, cs_int32 idx);
void luax_pushvec(lua_State* L, LuaVec* vec);
LuaAng* luax_checkang(lua_State* L, cs_int32 idx);

LuaVec* luax_newpsvec(lua_State* L, SVec* vec);
LuaVec* luax_newpfvec(lua_State* L, Vec* vec);
LuaAng* luax_newpang(lua_State* L, Ang* ang);
LuaAng* luax_newang(lua_State* L);
#endif
