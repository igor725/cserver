#include "core.h"
#include "client.h"
#include "platform.h"
#include "protocol.h"
#include "bot.h"

static ClientID FindFreeID(void) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++)
		if(!Clients_List[i]) return i;

	return CLIENT_SELF;
}

Client *Bot_New(void) {
	ClientID botid = FindFreeID();
	if(botid == CLIENT_SELF)
		return NULL;
	
	Client *client = Memory_Alloc(1, sizeof(Client));
	client->playerData = Memory_Alloc(1, sizeof(PlayerData));
	client->cpeData = Memory_Alloc(1, sizeof(CPEData));
	client->mutex = Mutex_Create();
	client->sock = INVALID_SOCKET;
	client->id = botid;

	Clients_List[botid] = client;
	return client;
}

cs_bool Bot_IsBot(Client *bot) {
	return bot->sock == INVALID_SOCKET;
}

cs_bool Bot_Spawn(Client *bot, World *world) {
	if(bot->playerData->world) return false;
	bot->playerData->world = world;
	for(ClientID id = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client || Bot_IsBot(client)) continue;
		if(Client_GetExtVer(client, EXT_PLAYERLIST))
			CPE_WriteAddEntity2(client, bot);
		else
			Vanilla_WriteSpawn(client, bot);
	}
	return true;
}
