#include <winsock2.h>
#include "core.h"
#include "world.h"
#include "client.h"
#include "event.h"

void Event_OnMessage(CLIENT* self, char* message, int len) {
	
}

bool Event_OnBlockPalce(CLIENT* self, ushort x, ushort y, ushort z, int id) {
	return true;
}

void Event_OnPlayerMove(CLIENT* self) {

}
