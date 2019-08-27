#ifndef EVENT_H
#define EVENT_H
void Event_OnSpawn(CLIENT* client);
void Event_OnDespawn(CLIENT* client);
void Event_OnHandshakeDone(CLIENT* client);
bool Event_OnMessage(CLIENT* client, char* message, int len);
void Event_OnHeldBlockChange(CLIENT* client, BlockID prev, BlockID curr);
bool Event_OnBlockPlace(CLIENT* client, ushort x, ushort y, ushort z, int id);
#endif
