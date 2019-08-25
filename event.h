#ifndef EVENT_H
#define EVENT_H
void Event_OnPlayerMove(CLIENT* self);
void Event_OnMessage(CLIENT* self, char* message, int len);
bool Event_OnBlockPalce(CLIENT* self, ushort x, ushort y, ushort z, int id);
#endif
