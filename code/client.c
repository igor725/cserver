#include "core.h"
#include "world.h"
#include "client.h"
#include "server.h"
#include "packets.h"
#include "cpe.h"
#include "event.h"
#include "config.h"

bool Client_Add(CLIENT client) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		if(!Clients_List[i]) {
			client->id = i;
			Clients_List[i] = client;
			return true;
		}
	}
	return false;
}

CLIENT Client_GetByName(const char* name) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT client = Clients_List[i];
		if(!client || !client->playerData) continue;
		if(String_CaselessCompare(client->playerData->name, name))
			return client;
	}
	return NULL;
}

CLIENT Client_GetByID(ClientID id) {
	return id < MAX_CLIENTS ? Clients_List[id] : NULL;
}

bool Client_Despawn(CLIENT client) {
	if(!client->playerData) return false;
	if(!client->playerData->spawned) return false;
	Packet_WriteDespawn(Client_Broadcast, client);
	client->playerData->spawned = false;
	Event_Call(EVT_ONDESPAWN, (void*)client);
	return true;
}

bool Client_ChangeWorld(CLIENT client, WORLD world) {
	if(Client_IsInWorld(client, world)) return false;

	Client_Despawn(client);
	Client_SetPos(client, world->info->spawnVec, world->info->spawnAng);
	if(!Client_SendMap(client, world)) {
		Client_Kick(client, "Map sending failed");
		return false;
	}
	return true;
}

void Client_Chat(CLIENT client, MessageType type, const char* message) {
	Packet_WriteChat(client, type, message);
}

static void HandlePacket(CLIENT client, PACKET packet, bool extended) {
	char* data = client->rdbuf + 1;
	bool ret = false;

	if(extended)
		if(!packet->cpeHandler)
			ret = packet->handler(client, data);
		else
			ret = packet->cpeHandler(client, data);
	else
		if(packet->handler)
			ret = packet->handler(client, data);

	if(!ret) Client_Kick(client, "Packet reading error");
}

TRET Client_ThreadProc(TARG lpParam) {
	CLIENT client = (CLIENT)lpParam;

	short packetSize = 0, wait = 1;
	bool extended = false;
	PACKET packet = NULL;

	while(1) {
		if(client->closed) {
			int len = recv(client->sock, client->rdbuf, 131, 0);
			if(len <= 0) {
				if(client->playerData && client->playerData->state > STATE_WLOADDONE)
					Event_Call(EVT_ONDISCONNECT, (void*)client);
				Socket_Close(client->sock);
				Client_Despawn(client);
				Client_Free(client);
				break;
			}
			continue;
		}

		if(wait > 0) {
			int len = recv(client->sock, client->rdbuf + client->bufpos, wait, 0);

			if(len > 0) {
				client->bufpos += (uint16_t)len;
			} else {
				Client_Disconnect(client);
			}
		}

		if(client->bufpos == 1) {
			packet = Packet_Get(*client->rdbuf);
			if(!packet) {
				Client_Kick(client, "Invalid packet ID");
				continue;
			}

			packetSize = packet->size;
			if(packet->haveCPEImp) {
				extended = Client_IsSupportExt(client, packet->extName, packet->extVersion);
				if(extended) packetSize = packet->extSize;
			}

			wait = packetSize - client->bufpos;
		}

		if(client->bufpos == packetSize) {
			HandlePacket(client, packet, extended);
			client->pps += 1;
			client->bufpos = 0;
			extended = false;
			wait = 1;
			continue;
		}
	}

	return 0;
}

TRET Client_MapThreadProc(TARG lpParam) {
	CLIENT client = (CLIENT)lpParam;
	WORLD world = client->playerData->world;
	Mutex_Lock(client->mutex);

	z_stream stream = {0};
	uint8_t* data = (uint8_t*)client->wrbuf;
	*data = 0x03;
	uint16_t* len = (uint16_t*)++data;
	uint8_t* out = data + 2;
	int ret;

	uint8_t* mapdata = world->data;
	int maplen = world->size;
	int windowBits = 31;

	if(client->cpeData && client->cpeData->fmSupport) {
		windowBits = -15;
		maplen -= 4;
		mapdata += 4;
	}

	if((ret = deflateInit2(
		&stream,
		Z_BEST_COMPRESSION,
		Z_DEFLATED,
		windowBits,
		8,
		Z_DEFAULT_STRATEGY)) != Z_OK) {
		client->playerData->state = STATE_WLOADERR;
		return 0;
	}

	stream.avail_in = maplen;
	stream.next_in = mapdata;

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			client->playerData->state = STATE_WLOADERR;
			deflateEnd(&stream);
			return 0;
		}

		*len = htons(1024 - (uint16_t)stream.avail_out);
		if(!Client_Send(client, 1028)) {
			client->playerData->state = STATE_WLOADERR;
			deflateEnd(&stream);
			return 0;
		}
	} while(stream.avail_out == 0);

	deflateEnd(&stream);
	Mutex_Unlock(client->mutex);
	Packet_WriteLvlFin(client);
	client->playerData->state = STATE_WLOADDONE;
	Client_Spawn(client);
	return 0;
}

void Client_Init(void) {
	Client_Broadcast = Memory_Alloc(1, sizeof(struct client));
	Client_Broadcast->wrbuf = Memory_Alloc(2048, 1);
	Client_Broadcast->mutex = Mutex_Create();
}

bool Client_IsInGame(CLIENT client) {
	return client->playerData && client->playerData->state == STATE_INGAME;
}

bool Client_IsInSameWorld(CLIENT client, CLIENT other) {
	return client->playerData->world == other->playerData->world;
}

bool Client_IsInWorld(CLIENT client, WORLD world) {
	return client->playerData && client->playerData->world == world;
}

bool Client_IsSupportExt(CLIENT client, const char* extName, int extVer) {
	if(!client->cpeData) return false;

	EXT ptr = client->cpeData->headExtension;
	while(ptr) {
		if(String_CaselessCompare(ptr->name, extName)) {
			return ptr->version == extVer;
		}
		ptr = ptr->next;
	}
	return false;
}

const char* Client_GetName(CLIENT client) {
	if(!client->playerData) return "unconnected";
	return client->playerData->name;
}

const char* Client_GetAppName(CLIENT client) {
	if(!client->cpeData) return "vanilla";
	return client->cpeData->appName;
}

//TODO: ClassiCube auth and saved playerdata reading
bool Client_CheckAuth(CLIENT client) {
	return true;
}

void Client_SetPos(CLIENT client, VECTOR pos, ANGLE ang) {
	if(!client->playerData) return;
	Memory_Copy(client->playerData->position, pos, sizeof(struct vector));
	Memory_Copy(client->playerData->angle, ang, sizeof(struct angle));
}

bool Client_SetProperty(CLIENT client, uint8_t property, int value) {
	if(Client_IsSupportExt(client, "EnvMapAspect", 1)) {
		CPEPacket_WriteMapProperty(client, property, value);
		return true;
	}
	return false;
}

bool Client_SetTexturePack(CLIENT client, const char* url) {
	if(Client_IsSupportExt(client, "EnvMapAspect", 1)) {
		CPEPacket_WriteTexturePack(client, url);
		return true;
	}
	return false;
}

bool Client_SetWeather(CLIENT client, Weather type) {
	if(Client_IsSupportExt(client, "EnvWeatherType", 1)) {
		CPEPacket_WriteWeatherType(client, type);
		return true;
	}
	return false;
}

bool Client_SetType(CLIENT client, bool isOP) {
	if(!client->playerData) return false;
	client->playerData->isOP = isOP;
	Packet_WriteUpdateType(client);
	return true;
}

bool Client_SetHotbar(CLIENT client, Order pos, BlockID block) {
	if(!Block_IsValid(block) || pos > 8) return false;
	if(Client_IsSupportExt(client, "SetHotbar", 1)) {
		CPEPacket_WriteSetHotBar(client, pos, block);
		return true;
	}
	return false;
}

bool Client_SetBlockPerm(CLIENT client, BlockID block, bool allowPlace, bool allowDestroy) {
	if(!Block_IsValid(block)) return false;
	if(Client_IsSupportExt(client, "BlockPermissions", 1)) {
		CPEPacket_WriteBlockPerm(client, block, allowPlace, allowDestroy);
		return true;
	}
	return false;
}

bool Client_SetModel(CLIENT client, const char* model) {
	if(!client->cpeData) return false;
	if(!CPE_CheckModel(model)) return false;
	String_Copy(client->cpeData->model, 16, model);

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT other = Clients_List[i];
		if(!other || Client_IsSupportExt(other, "SetModel", 1)) continue;
		CPEPacket_WriteSetModel(other, other == client ? 0xFF : client->id, model);
	}

	return true;
}

bool Client_GetType(CLIENT client) {
	return client->playerData ? client->playerData->isOP : false;
}

void Client_Free(CLIENT client) {
	Clients_List[client->id] = NULL;
	Memory_Free(client->rdbuf);
	Memory_Free(client->wrbuf);

	if(client->thread) Thread_Close(client->thread);
	if(client->mapThread) Thread_Close(client->mapThread);
	if(client->mutex) Mutex_Free(client->mutex);

	if(client->playerData) {
		Memory_Free((void*)client->playerData->name);
		Memory_Free((void*)client->playerData->key);
		Memory_Free(client->playerData->position);
		Memory_Free(client->playerData->angle);
		Memory_Free(client->playerData);
	}

	if(client->cpeData) {
		EXT prev, ptr = client->cpeData->headExtension;

		while(ptr) {
			prev = ptr;
			Memory_Free((void*)ptr->name);
			ptr = ptr->next;
			Memory_Free(prev);
		}
		Memory_Free((void*)client->cpeData->appName);
		Memory_Free(client->cpeData);
	}

	Memory_Free(client);
}

int Client_Send(CLIENT client, int len) {
	if(client == Client_Broadcast) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			CLIENT bClient = Clients_List[i];

			if(bClient) {
				Mutex_Lock(bClient->mutex);
				Socket_Send(bClient->sock, Client_Broadcast->wrbuf, len);
				Mutex_Unlock(bClient->mutex);
			}
		}
		return len;
	}

	return Socket_Send(client->sock, client->wrbuf, len) == len;
}

bool Client_Spawn(CLIENT client) {
	if(client->playerData->spawned) return false;
	WORLD world = client->playerData->world;

	Client_SetWeather(client, world->info->wt);
	for(uint8_t prop = 0; prop < WORLD_PROPS_COUNT; prop++) {
		Client_SetProperty(client, prop, World_GetProperty(world, prop));
	}

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT other = Clients_List[i];
		if(other && Client_IsInSameWorld(client, other)) {
			Packet_WriteSpawn(other, client);

			if(other->cpeData && client->cpeData)
				CPEPacket_WriteSetModel(other, other == client ? 0xFF : client->id, client->cpeData->model);

			if(client != other) {
				Packet_WriteSpawn(client, other);

				if(other->cpeData && client->cpeData)
					CPEPacket_WriteSetModel(client, other->id, other->cpeData->model);
			}
		}
	}

	client->playerData->spawned = true;
	Event_Call(EVT_ONSPAWN, (void*)client);
	return true;
}

bool Client_SendMap(CLIENT client, WORLD world) {
	if(client->mapThread) return false;

	client->playerData->state = STATE_MOTD;
	client->playerData->world = world;
	Packet_WriteLvlInit(client);
	client->mapThread = Thread_Create(Client_MapThreadProc, client);
	return true;
}

void Client_HandshakeStage2(CLIENT client) {
	Client_ChangeWorld(client, Worlds_List[0]);
}

void Client_Disconnect(CLIENT client) {
	Client_Despawn(client);
	Socket_Shutdown(client->sock, SD_SEND);
	client->closed = true;
}

void Client_Kick(CLIENT client, const char* reason) {
	if(!reason) reason = "Kicked without reason";
	Packet_WriteKick(client, reason);
	Client_Disconnect(client);
}

void Client_UpdatePositions(CLIENT client) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT other = Clients_List[i];
		if(other && client != other && Client_IsInGame(other) && Client_IsInSameWorld(client, other))
			Packet_WritePosAndOrient(other, client);
	}
}

void Client_Tick(CLIENT client) {
	if(!client->playerData) return;

	if(client->ppstm < 1000) {
		client->ppstm += Server_Delta;
	} else {
		if(client->pps > MAX_CLIENT_PPS) {
			Client_Kick(client, "Too many packets per second");
			return;
		}
		client->pps = 0;
		client->ppstm = 0;
	}

	switch (client->playerData->state) {
		case STATE_WLOADDONE:
			client->playerData->state = STATE_INGAME;
			Thread_Close(client->mapThread);
			client->mapThread = NULL;
			break;
		case STATE_WLOADERR:
			Client_Kick(client, "Map loading error");
			break;
	}
}
