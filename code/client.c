#include "core.h"
#include "zlib.h"
#include "world.h"
#include "client.h"
#include "packets.h"
#include "cpe.h"
#include "event.h"

ClientID Client_FindFreeID() {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		if(!clients[i])
			return i;
	}
	return (ClientID)-1;
}

CLIENT* Client_FindByName(const char* name) {
	for(int i = 0; i < MAX_CLIENTS; i++) {
		CLIENT* client = clients[i];
		if(!client || !client->playerData) continue;
		if(String_CaselessCompare(client->playerData->name, name)) {
			return client;
		}
	}
	return NULL;
}

bool Client_Despawn(CLIENT* client) {
	if(!client->playerData)
		return false;

	if(!client->playerData->spawned)
		return false;

	Packet_WriteDespawn(Broadcast, client);
	client->playerData->spawned = false;
	Event_OnDespawn(client);
	return true;
}

bool Client_ChangeWorld(CLIENT* client, WORLD* world) {
	if(!client->playerData) return false;
	if(client->playerData->currentWorld == world) return true;

	Client_Despawn(client);
	Client_SetPos(client, world->info->spawnVec, world->info->spawnAng);
	if(!Client_SendMap(client, world)) {
		Client_Kick(client, "Map sending failed");
		return false;
	}

	return true;
}

TRET Client_ThreadProc(TARG lpParam) {
	CLIENT* client = (CLIENT*)lpParam;

	while(1) {
		if(client->status == CLIENT_WAITCLOSE) {
			int len = recv(client->sock, client->rdbuf, 131, 0);
			if(len <= 0) {
				Event_OnDisconnect(client);
				Socket_Close(client->sock);
				Client_Despawn(client);
				Client_Destroy(client);
				break;
			}
			continue;
		}

		ushort wait = 1;

		if(client->bufpos > 0) {
			short packetSize = Packet_GetSize(*client->rdbuf, client);

			if(packetSize < 1) {
				Client_Kick(client, "Invalid packet ID");
				continue;
			}
			wait = packetSize - client->bufpos;
		}

		if(wait == 0) {
			Client_HandlePacket(client);
			client->bufpos = 0;
			continue;
		} else if(wait > 0) {
			int len = recv(client->sock, client->rdbuf + client->bufpos, wait, 0);

			if(len > 0) {
				client->bufpos += (ushort)len;
			} else {
				Client_Disconnect(client);
			}
		}
	}

	return 0;
}

TRET Client_MapThreadProc(TARG lpParam) {
	CLIENT* client = (CLIENT*)lpParam;
	WORLD* world = client->playerData->currentWorld;

	z_stream stream = {0};
	uchar* data = (uchar*)client->wrbuf;
	*data = 0x03;
	ushort* len = (ushort*)++data;
	uchar* out = data + 2;
	int ret;

	uchar* mapdata = world->data;
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

		*len = htons(1024 - (ushort)stream.avail_out);
		if(!Client_Send(client, 1028)) {
			client->playerData->state = STATE_WLOADERR;
			deflateEnd(&stream);
			return 0;
		}
	} while(stream.avail_out == 0);

	deflateEnd(&stream);
	Packet_WriteLvlFin(client);
	client->playerData->state = STATE_WLOADDONE;
	Client_Spawn(client);
	return 0;
}

void Client_Init() {
	Broadcast = Memory_Alloc(1, sizeof(CLIENT));
	Broadcast->wrbuf = Memory_Alloc(2048, 1);
	Broadcast->mutex = Mutex_Create();
}

void Client_UpdateBlock(CLIENT* client, WORLD* world, ushort x, ushort y, ushort z) {
	BlockID block = World_GetBlock(world, x, y, z);

	for(int i = 0; i < MAX_CLIENTS; i++) {
		CLIENT* other = clients[i];
		if(!other || other == client) continue;
		if(!other->playerData || other->playerData->currentWorld != world) continue;
		if(other->playerData->state != STATE_INGAME) continue;
		Packet_WriteSetBlock(other, x, y, z, block);
	}
}

bool Client_IsSupportExt(CLIENT* client, const char* extName) {
	if(!client->cpeData)
		return false;

	EXT* ptr = client->cpeData->headExtension;
	while(ptr) {
		if(String_CaselessCompare(ptr->name, extName))
			return true;
		ptr = ptr->next;
	}
	return false;
}

const char* Client_GetName(CLIENT* client) {
	if(!client->playerData)
		return "unconnected";
	return client->playerData->name;
}

const char* Client_GetAppName(CLIENT* client) {
	if(!client->cpeData)
		return "vanilla";
	return client->cpeData->appName;
}

//TODO: ClassiCube auth and saved playerdata reading
bool Client_CheckAuth(CLIENT* client) {
	return true;
}

void Client_SetPos(CLIENT* client, VECTOR* pos, ANGLE* ang) {
	if(!client->playerData)
		return;
	Memory_Copy(client->playerData->position, pos, sizeof(VECTOR));
	Memory_Copy(client->playerData->angle, ang, sizeof(ANGLE));
}

bool Client_SetType(CLIENT* client, bool isOP) {
	if(!client->playerData)
		return false;
	client->playerData->isOP = isOP;
	Packet_WriteUpdateType(client);
	return true;
}

bool Client_SetHotbar(CLIENT* client, Order pos, BlockID block) {
	if(!Block_IsValid(block) || pos > 8) return false;
	if(Client_IsSupportExt(client, "SetHotbar")) {
		CPEPacket_WriteSetHotBar(client, pos, block);
		return true;
	}
	return false;
}

bool Client_GetType(CLIENT* client) {
	if(!client->playerData)
		return false;
	return client->playerData->isOP;
}

void Client_Destroy(CLIENT* client) {
	clients[client->id] = NULL;
	Memory_Free(client->rdbuf);
	Memory_Free(client->wrbuf);

	if(client->thread)
		Thread_Close(client->thread);

	if(client->mutex)
		Mutex_Free(client->mutex);

	if(client->playerData) {
		Memory_Free((void*)client->playerData->name);
		Memory_Free((void*)client->playerData->key);
		Memory_Free(client->playerData->position);
		Memory_Free(client->playerData->angle);
		Memory_Free(client->playerData);
	}

	if(client->cpeData) {
		EXT* ptr = client->cpeData->headExtension;
		EXT* prev;

		while(ptr) {
			prev = ptr;
			Memory_Free((void*)ptr->name);
			ptr = ptr->next;
			Memory_Free(prev);
		}
		Memory_Free(client->cpeData->appName);
		Memory_Free(client->cpeData);
	}

	Memory_Free(client);
}

int Client_Send(CLIENT* client, int len) {
	if(client == Broadcast) {
		for(int i = 0; i < MAX_CLIENTS; i++) {
			CLIENT* bClient = clients[i];

			if(bClient) {
				Mutex_Lock(bClient->mutex);
				send(bClient->sock, Broadcast->wrbuf, len, 0);
				Mutex_Unlock(bClient->mutex);
			}
		}
		return len;
	}

	return send(client->sock, client->wrbuf, len, 0) == len;
}

bool Client_Spawn(CLIENT* client) {
	if(client->playerData->spawned)
		return false;

	for(int i = 0; i < MAX_CLIENTS; i++) {
		CLIENT* other = clients[i];
		if(!other) continue;

		Packet_WriteSpawn(other, client);
		if(client != other)
			Packet_WriteSpawn(client, other);
	}

	client->playerData->spawned = true;
	Event_OnSpawn(client);
	return true;
}

bool Client_SendMap(CLIENT* client, WORLD* world) {
	if(client->playerData->mapThread)
		return false;

	client->playerData->state = STATE_MOTD;
	client->playerData->currentWorld = world;
	Packet_WriteLvlInit(client);
	client->playerData->mapThread = Thread_Create((TFUNC)&Client_MapThreadProc, client);
	if(!Thread_IsValid(client->playerData->mapThread)) {
		Client_Kick(client, "Can't create map sending thread");
		return false;
	}

	return true;
}

void Client_HandshakeStage2(CLIENT* client) {
	Client_ChangeWorld(client, worlds[0]);
}

void Client_Disconnect(CLIENT* client) {
	Client_Despawn(client);
	shutdown(client->sock, SD_SEND);
	client->status = CLIENT_WAITCLOSE;
}

void Client_Kick(CLIENT* client, const char* reason) {
	Packet_WriteKick(client, reason);
	Client_Disconnect(client);
}

void Client_UpdatePositions(CLIENT* client) {
	if(!client->playerData->positionUpdated)
		return;
	client->playerData->positionUpdated = false;

	for(int i = 0; i < MAX_CLIENTS; i++) {
		CLIENT* other = clients[i];
		if(!other || !other->playerData || other->playerData->state != STATE_INGAME || client == other) continue;
		if(client->playerData->currentWorld != other->playerData->currentWorld) continue;
		Packet_WritePosAndOrient(other, client);
	}
}

void Client_Tick(CLIENT* client) {
	if(!client->playerData) return;
	switch (client->playerData->state) {
		case STATE_WLOADDONE:
			Thread_Close(client->playerData->mapThread);
			client->playerData->state = STATE_INGAME;
			client->playerData->mapThread = NULL;
			break;
		case STATE_WLOADERR:
			Client_Kick(client, "Map loading error");
			break;
		case STATE_INGAME:
			Client_UpdatePositions(client);
			break;
	}
}

void Client_HandlePacket(CLIENT* client) {
	char* data = client->rdbuf;
	uchar id = *data; ++data;
	PACKET* packet = packets[id];
	bool ret = false;

	if(packet->haveCPEImp && Client_IsSupportExt(client, packet->extName))
		if(!packet->cpeHandler)
			ret = packet->handler(client, data);
		else
			ret = packet->cpeHandler(client, data);
	else
		if(packet->handler)
			ret = packet->handler(client, data);

	if(!ret)
		Client_Kick(client, "Packet reading error");
}
