#include "core.h"
#include "str.h"
#include "client.h"
#include "protocol.h"
#include "server.h"
#include "bot.h"

Client* Bot_New(const char* name, World* world, cs_bool hideName) {
	ClientID id;
	for(id = -127; id < 0; id++) {
		if(!Bots_List[id]) break;
	}
	if(id == 0) return NULL;

	Client* bot = Client_New(0, INADDR_LOOPBACK);
	bot->cpeData = Memory_Alloc(1, sizeof(CPEData));
	bot->cpeData->appName = SOFTWARE_FULLNAME;
	bot->cpeData->hideDisplayName = hideName;
	bot->playerData = Memory_Alloc(1, sizeof(PlayerData));
	bot->playerData->name = String_AllocCopy(name);
	bot->playerData->firstSpawn = true;
	bot->playerData->world = world;
	bot->cpeData->model = 256;
	bot->closed = true;
	bot->id = id;
	Bots_List[id] = bot;

	WorldInfo* wi = &world->info;
	Bot_SetPosition(bot, &wi->spawnVec, &wi->spawnAng);

	return bot;
}

void Bot_SetPosition(Client* bot, Vec* pos, Ang* ang) {
	bot->playerData->position = *pos;
	bot->playerData->angle = *ang;
}

void Bot_SetSkin(Client* bot, const char* skin) {
	Client_SetSkin(bot, skin);
}

void Bot_SetModel(Client* bot, cs_int16 model) {
	bot->cpeData->model = model;
	bot->cpeData->updates |= PCU_MODEL;
}

cs_int16 Bot_GetModel(Client* bot) {
	if(!bot->cpeData) return 256;
	return bot->cpeData->model;
}

void Bot_UpdatePosition(Client* bot) {
	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client* client = Clients_List[id];
		if(client && Client_IsInSameWorld(client, bot))
			Vanilla_WritePosAndOrient(client, bot);
	}
}

cs_bool Bot_SpawnFor(Client* bot, Client* client) {
	if(!Client_IsInSameWorld(client, bot)) return false;
	cs_int32 extlist_ver = Client_GetExtVer(client, EXT_PLAYERLIST);

	if(extlist_ver == 2) {
		CPE_WriteAddEntity2(client, bot);
	} else {
		Vanilla_WriteSpawn(client, bot);
	}

	return true;
}

cs_bool Bot_Spawn(Client* bot) {
	if(bot->playerData->spawned) return false;
	bot->playerData->spawned = true;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client* client = Clients_List[id];
		if(client)
			Bot_SpawnFor(bot, client);
	}

	Bot_Update(bot);
	return true;
}

cs_bool Bot_Despawn(Client* bot) {
	if(!bot->playerData->spawned) return false;
	bot->playerData->spawned = false;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client* client = Clients_List[id];
		if(client && Client_IsInSameWorld(client, bot))
			Vanilla_WriteDespawn(client, bot);
	}

	return true;
}

cs_bool Bot_Update(Client* bot) {
	return Client_Update(bot);
}

void Bot_Free(Client* bot) {
	if(bot->id < 0)
		Bots_List[-bot->id] = NULL;

	Memory_Free(bot->cpeData);
	Memory_Free((void*)bot->playerData->name);
	Memory_Free(bot->playerData);
	Memory_Free(bot);
}
