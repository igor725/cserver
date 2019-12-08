#ifndef LWORLD_H
#define LWORLD_H
World* luax_checkworld(lua_State* L, cs_int32 idx);
void luax_pushworld(lua_State* L, World* world);
#endif
