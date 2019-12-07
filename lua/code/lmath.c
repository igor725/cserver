#include <core.h>
#include <vector.h>
#include <world.h>

#include "script.h"
#include "lmath.h"

#define LUA_TVECTOR "Vector"
#define LUA_TANGLE "Angle"

LuaAng* luax_newang(lua_State* L) {
	return luax_newobject(L, sizeof(LuaAng), LUA_TANGLE);
}

LuaVec* luax_newvec(lua_State* L) {
	return luax_newobject(L, sizeof(LuaVec), LUA_TVECTOR);
}

LuaVec* luax_newpsvec(lua_State* L, SVec* vec) {
	LuaVec* lvec = luax_newobject(L, sizeof(LuaVec), LUA_TVECTOR);
	lvec->type = VT_SHORT;
	lvec->allocated = false;
	lvec->value.s = vec;
	return lvec;
}

LuaVec* luax_newpfvec(lua_State* L, Vec* vec) {
	LuaVec* lvec = luax_newobject(L, sizeof(LuaVec), LUA_TVECTOR);
	lvec->type = VT_FLOAT;
	lvec->allocated = false;
	lvec->value.f = vec;
	return lvec;
}

LuaAng* luax_newpang(lua_State* L, Ang* ang) {
	LuaAng* lang = luax_newobject(L, sizeof(LuaAng), LUA_TANGLE);
	lang->allocated = false;
	lang->value = ang;
	return lang;
}

LuaVec* luax_checkvec(lua_State* L, cs_int32 idx) {
	return luax_checkobject(L, idx, LUA_TVECTOR);
}

LuaAng* luax_checkang(lua_State* L, cs_int32 idx) {
	return luax_checkobject(L, idx, LUA_TANGLE);
}

#define VEC_TYPEERROR "Vector object on #%d must have type %q."

SVec* luax_checksvec(lua_State* L, cs_int32 idx) {
	const LuaVec* lvec = luax_checkvec(L, idx);
	if(lvec->type != VT_SHORT) luaL_error(L, VEC_TYPEERROR, idx, "short");
	return lvec->value.s;
}

Vec* luax_checkfvec(lua_State* L, cs_int32 idx) {
	const LuaVec* lvec = luax_checkvec(L, idx);
	if(lvec->type != VT_FLOAT) luaL_error(L, VEC_TYPEERROR, idx, "float");
	return lvec->value.f;
}

LUA_SFUNC(mvec_set) {
 	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT) {
		cs_int16 x = (cs_int16)luaL_checkinteger(L, 2),
		y = (cs_int16)luaL_checkinteger(L, 3),
		z = (cs_int16)luaL_checkinteger(L, 4);
		Vec_Set(*vec->value.s, x, y, z);
	} else {
		float x = (float)luaL_checknumber(L, 2),
		y = (float)luaL_checknumber(L, 3),
		z = (float)luaL_checknumber(L, 4);
		Vec_Set(*vec->value.f, x, y, z);
	}

	lua_pushvalue(L, 1);
	return 1;
}

LUA_SFUNC(mvec_copy) {
	LuaVec* vec1 = luax_checkvec(L, 1);
	LuaVec* vec2 = luax_checkvec(L, 2);
	if(vec1->type != vec2->type) luaL_error(L, "Passed vectors have different types");

	if(vec1->type == VT_SHORT)
		*vec1->value.s = *vec2->value.s;
	else
		*vec1->value.f = *vec2->value.f;

	lua_pushvalue(L, 1);
	return 1;
}

LUA_SFUNC(mvec_get) {
	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT) {
		SVec* svec = vec->value.s;
		lua_pushinteger(L, svec->x);
		lua_pushinteger(L, svec->y);
		lua_pushinteger(L, svec->z);
	} else {
		Vec* fvec = vec->value.f;
		lua_pushnumber(L, fvec->x);
		lua_pushnumber(L, fvec->y);
		lua_pushnumber(L, fvec->z);
	}

	return 3;
}

LUA_SFUNC(mvec_getx) {
	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT)
		lua_pushinteger(L, vec->value.s->x);
	else
		lua_pushnumber(L, vec->value.f->x);

	return 1;
}

LUA_SFUNC(mvec_gety) {
	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT)
		lua_pushinteger(L, vec->value.s->y);
	else
		lua_pushnumber(L, vec->value.f->y);

	return 1;
}

LUA_SFUNC(mvec_getz) {
	LuaVec* vec = luax_checkvec(L, 1);

	if(vec->type == VT_SHORT)
		lua_pushinteger(L, vec->value.s->z);
	else
		lua_pushnumber(L, vec->value.f->z);

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

LUA_SFUNC(mvec_eq) {
	LuaVec* vec1 = luax_checkvec(L, 1);
	LuaVec* vec2 = luax_checkvec(L, 2);
	if(vec1->type != vec2->type) luaL_error(L, "Passed vectors have different types");

	if(vec1->type == VT_SHORT)
		lua_pushboolean(L, SVec_Compare(vec1->value.s, vec2->value.s));
	else
		lua_pushboolean(L, Vec_Compare(vec1->value.f, vec2->value.f));
	return 1;
}

LUA_SFUNC(mvec_gc) {
	LuaVec* vec = luax_checkvec(L, 1);
	if(vec->allocated)
		Memory_Free(vec->value.p);
	return 0;
}

LUA_SFUNC(mang_set) {
 	LuaAng* ang = luax_checkang(L, 1);

	float yaw = (float)luaL_checknumber(L, 2),
	pitch = (float)luaL_checknumber(L, 3);
	Ang_Set(*ang->value, yaw, pitch);

	lua_pushvalue(L, 1);
	return 1;
}

LUA_SFUNC(mang_copy) {
	LuaAng* ang1 = luax_checkang(L, 1);
	LuaAng* ang2 = luax_checkang(L, 2);
	*ang1->value = *ang2->value;

	lua_pushvalue(L, 1);
	return 1;
}

LUA_SFUNC(mang_get) {
	LuaAng* lang = luax_checkang(L, 1);
	Ang* ang = lang->value;
	lua_pushnumber(L, ang->yaw);
	lua_pushnumber(L, ang->pitch);

	return 2;
}

LUA_SFUNC(mang_getyaw) {
	LuaAng* ang = luax_checkang(L, 1);

	lua_pushnumber(L, ang->value->yaw);
	return 1;
}

LUA_SFUNC(mang_getpitch) {
	LuaAng* ang = luax_checkang(L, 1);

	lua_pushnumber(L, ang->value->pitch);
	return 1;
}

static const luaL_Reg ang_methods[] = {
	{"set", mang_set},
	{"copy", mang_copy},

	{"get", mang_get},
	{"getyaw", mang_getyaw},
	{"getpitch", mang_getpitch},

	{NULL, NULL}
};

LUA_SFUNC(mang_eq) {
	LuaAng* ang1 = luax_checkang(L, 1);
	LuaAng* ang2 = luax_checkang(L, 2);

	lua_pushboolean(L, Ang_Compare(ang1->value, ang2->value));
	return 1;
}

LUA_SFUNC(mang_gc) {
	LuaAng* ang = luax_checkang(L, 1);
	if(ang->allocated)
		Memory_Free(ang->value);
	return 0;
}

LUA_SFUNC(fvec_new) {
	cs_int8 type = (cs_int8)lua_toboolean(L, 1);
	LuaVec* vec = luax_newvec(L);
	vec->type = type;
	vec->allocated = true;
	vec->value.p = Memory_Alloc(1, type == false ? sizeof(struct _SVec) : sizeof(struct _Vec));
	return 1;
}

LUA_SFUNC(fang_new) {
	LuaAng* ang = luax_newang(L);
	ang->allocated = true;
	ang->value = Memory_Alloc(1, sizeof(struct _Ang));
	return 1;
}

static const luaL_Reg csmath_funcs[] = {
	{"vector", fvec_new},
	{"angle", fang_new},

	{NULL, NULL}
};

LUA_FUNC(luaopen_csmath) {
	luaL_newmetatable(L, LUA_TVECTOR);
	lua_newtable(L);
	luaL_setfuncs(L, vec_methods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, mvec_eq);
	lua_setfield(L, -2, "__eq");
	lua_pushcfunction(L, mvec_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newmetatable(L, LUA_TANGLE);
	lua_newtable(L);
	luaL_setfuncs(L, ang_methods, 0);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, mang_eq);
	lua_setfield(L, -2, "__eq");
	lua_pushcfunction(L, mang_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newlib(L, csmath_funcs);
	return 1;
}
