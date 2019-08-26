#ifndef EVENT_H
#define EVENT_H
void Event_OnHandshakeDone(CLIENT* client);
bool Event_OnMessage(CLIENT* client, char* message, int len);
bool Event_OnBlockPalce(CLIENT* client, ushort x, ushort y, ushort z, int id);
#endif
