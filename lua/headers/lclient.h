#ifndef LCLIENT_H
#define LCLIENT_H
Client luax_checkclient(lua_State* L, cs_int32 idx);
void luax_pushclient(lua_State* L, Client client);
#endif
