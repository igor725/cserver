#include <string.h>
#include "core.h"
#include "world.h"
#include "server.h"
#include "client.h"
#include "event.h"

bool Event_OnMessage(CLIENT* client, char* message, int len) {
	return true;
}

bool Event_OnBlockPlace(CLIENT* client, ushort x, ushort y, ushort z, int id) {
	return true;
}

void Event_OnHandshakeDone(CLIENT* client) {

}

void Event_OnSpawn(CLIENT* client) {

}

void Event_OnDespawn(CLIENT* client) {

}

void Event_OnHeldBlockChange(CLIENT* client, BlockID prev, BlockID curr) {

}
