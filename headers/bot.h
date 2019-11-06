#ifndef BOT_H
#define BOT_H
Client Bots_List[127];
API Client Bot_New(const char* name, World world, cs_bool hideName);
API cs_bool Bot_SpawnFor(Client bot, Client client);
API cs_bool Bot_Spawn(Client bot);
API cs_bool Bot_Despawn(Client bot);
API void Bot_SetPosition(Client bot, Vec* pos, Ang* ang);
API void Bot_SetModel(Client bot, cs_int16 model);
API void Bot_SetSkin(Client bot, const char* skin);
API void Bot_UpdatePosition(Client bot);
API cs_bool Bot_Update(Client bot);
API void Bot_Free(Client bot);
#endif
