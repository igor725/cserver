#include <core.h>
#include <vector.h>
#include <world.h>

#include "script.h"
#include "lvector.h"

#define LUA_TVECTOR "Vector"

LuaVec* luax_newvec(lua_State* L) {
	return luax_newmyobject(L, sizeof(LuaVec), LUA_TVECTOR);
}

LuaVec* luax_newpsvec(lua_State* L, SVec* vec) {
	LuaVec* lvec = luax_newmyobject(L, sizeof(LuaVec), LUA_TVECTOR);
	lvec->type = VT_SHORT;
	lvec->allocated = false;
	lvec->val.s = vec;
	return lvec;
}

LuaVec* luax_newpfvec(lua_State* L, Vec* vec) {
	LuaVec* lvec = luax_newmyobject(L, sizeof(LuaVec), LUA_TVECTOR);
	lvec->type = VT_FLOAT;
	lvec->allocated = false;
	lvec->val.f = vec;
	return lvec;
}

LuaVec* luax_checkvec(lua_State* L, cs_int32 idx) {
	return luax_checkobject(L, idx, LUA_TVECTOR);
}

#define VEC_TYPEERROR "Vector object #%d must have type %q."

SVec* luax_checksvec(lua_State* L, cs_int32 idx) {
	const LuaVec* lvec = luax_checkvec(L, idx);
	if(lvec->type != VT_SHORT) luaL_error(L, VEC_TYPEERROR, idx, "short");
	return lvec->val.s;
}

Vec* luax_checkfvec(lua_State* L, cs_int32 idx) {
	const LuaVec* lvec = luax_checkvec(L, idx);
	if(lvec->type != VT_FLOAT) luaL_error(L, VEC_TYPEERROR, idx, "float");
	return lvec->val.f;
}

LUA_SFUNC(mvec_set) {
 	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT) {
		cs_int16 x = (cs_int16)luaL_checkinteger(L, 2),
		y = (cs_int16)luaL_checkinteger(L, 3),
		z = (cs_int16)luaL_checkinteger(L, 4);
		Vec_Set(*vec->val.s, x, y, z);
	} else {
		float x = (float)luaL_checknumber(L, 2),
		y = (float)luaL_checknumber(L, 3),
		z = (float)luaL_checknumber(L, 4);
		Vec_Set(*vec->val.f, x, y, z);
	}

	lua_pushvalue(L, 1);
	return 1;
}

LUA_SFUNC(mvec_copy) {
	LuaVec* vec1 = luax_checkvec(L, 1);
	LuaVec* vec2 = luax_checkvec(L, 2);
	if(vec1->type != vec2->type) luaL_error(L, "Passed vectors have different types");

	if(vec1->type == VT_SHORT)
		*vec1->val.s = *vec2->val.s;
	else
		*vec1->val.f = *vec2->val.f;

	lua_pushvalue(L, 1);
	return 1;
}

LUA_SFUNC(mvec_get) {
	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT) {
		SVec* svec = vec->val.s;
		lua_pushinteger(L, svec->x);
		lua_pushinteger(L, svec->y);
		lua_pushinteger(L, svec->z);
	} else {
		Vec* fvec = vec->val.f;
		lua_pushnumber(L, fvec->x);
		lua_pushnumber(L, fvec->y);
		lua_pushnumber(L, fvec->z);
	}

	return 3;
}

LUA_SFUNC(mvec_getx) {
	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT)
		lua_pushinteger(L, vec->val.s->x);
	else
		lua_pushnumber(L, vec->val.f->x);

	return 1;
}

LUA_SFUNC(mvec_gety) {
	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT)
		lua_pushinteger(L, vec->val.s->y);
	else
		lua_pushnumber(L, vec->val.f->y);

	return 1;
}

LUA_SFUNC(mvec_getz) {
	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT)
		lua_pushinteger(L, vec->val.s->z);
	else
		lua_pushnumber(L, vec->val.f->z);

	return 1;
}

static const luaL_Reg vec_methods[] = {
	{"set", mvec_set},
	{"copy", mvec_copy},

	{"get", mvec_get},
	{"getx", mvec_getx},
	{"gety", mvec_gety},
	{"getz", mvec_getz},

	{NULL, NULL}
};

LUA_SFUNC(fvec_new) {
	cs_int8 type = (cs_int8)lua_toboolean(L, 1);
	LuaVec* vec = luax_newvec(L);
	vec->type = type;
	vec->allocated = true;
	vec->val.p = Memory_Alloc(1, type == false ? sizeof(struct _SVec) : sizeof(struct _Vec));
	return 1;
}

static const luaL_Reg vec_funcs[] = {
	{"new", fvec_new},

	{NULL, NULL}
};

LUA_SFUNC(mvec_eq) {
	LuaVec* vec1 = luax_checkvec(L, 1);
	LuaVec* vec2 = luax_checkvec(L, 2);
	if(vec1->type != vec2->type) luaL_error(L, "Passed vectors have different types");

	if(vec1->type == VT_SHORT)
		lua_pushboolean(L, SVec_Compare(vec1->val.s, vec2->val.s));
	else
		lua_pushboolean(L, Vec_Compare(vec1->val.f, vec2->val.f));
	return 1;
}

LUA_SFUNC(mvec_gc) {
	LuaVec* vec = luax_checkvec(L, 1);
	if(vec->allocated)
		Memory_Free(vec->val.p);
	return 0;
}

LUA_FUNC(luaopen_vector) {
	luaL_newmetatable(L, LUA_TVECTOR);
	lua_newtable(L);
	luaL_setfuncs(L, vec_methods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, mvec_eq);
	lua_setfield(L, -2, "__eq");
	lua_pushcfunction(L, mvec_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newlib(L, vec_funcs);
	return 1;
}
