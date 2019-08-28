#include "core.h"
#include "zlib.h"
#include "world.h"
#include "client.h"
#include "packets.h"
#include "cpe.h"
#include "event.h"
#include "platform.h"

ClientID Client_FindFreeID() {
	for(ClientID i = 0; i < 128; i++) {
		if(!clients[i])
			return i;
	}
	return -1;
}

CLIENT* Client_FindByName(const char* name) {
	for(int i = 0; i < 128; i++) {
		CLIENT* client = clients[i];
		if(!client) continue;
		if(_stricmp(client->playerData->name, name) == 0) {
			return client;
		}
	}
	return NULL;
}

THRET Client_ThreadProc(void* lpParam) {
	CLIENT* client = (CLIENT*)lpParam;

	while(1) {
		if(client->status == CLIENT_WAITCLOSE) {
			int len = recv(client->sock, client->rdbuf, 131, 0);
			if(len <= 0) {
				client->status = CLIENT_AFTERCLOSE;
				Socket_Close(client->sock);
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
				client->bufpos += len;
			} else {
				client->status = CLIENT_AFTERCLOSE;
				break;
			}
		}
	}

	return 0;
}

THRET Client_MapThreadProc(void* lpParam) {
	CLIENT* client = (CLIENT*)lpParam;
	WORLD* world = client->playerData->currentWorld;

	z_stream stream = {0};

	client->wrbuf[0] = 0x03;
	client->wrbuf[1027] = 0;
	ushort* len = (ushort*)(client->wrbuf + 1);
	uchar* out = (uchar*)len + 2;
	int ret;

	if((ret = deflateInit2_(
		&stream,
		4,
		Z_DEFLATED,
		31,
		8,
		Z_DEFAULT_STRATEGY,
		zlibVersion(),
		sizeof(stream)
	)) != Z_OK) {
		client->playerData->state = STATE_WLOADERR;
		return 0;
	}

	stream.avail_in = world->size;
	stream.next_in = (uchar*)world->data;
	*(uint*)world->data = htonl(world->size - 4);

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			client->playerData->state = STATE_WLOADERR;
			deflateEnd(&stream);
			return 0;
		}

		*len = htons(1024 - stream.avail_out);
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
	Broadcast = calloc(1, sizeof(struct client));
	Broadcast->wrbuf = calloc(2048, 1);
}

void Client_UpdateBlock(CLIENT* client, WORLD* world, ushort x, ushort y, ushort z) {
	BlockID block = World_GetBlock(world, x, y, z);

	for(int i = 0; i < 128; i++) {
		CLIENT* other = clients[i];
		if(!other || other == client) continue;
		if(!other->playerData || other->playerData->currentWorld != world) continue;
		Packet_WriteSetBlock(other, x, y, z, block);
	}
}

bool Client_IsSupportExt(CLIENT* client, const char* extName) {
	if(!client->cpeData)
		return false;

	EXT* ptr = client->cpeData->firstExtension;
	while(ptr) {
		if(_stricmp(ptr->name, extName) == 0) {
			return true;
		}
		ptr = ptr->next;
	}
	return false;
}

char* Client_GetAppName(CLIENT* client) {
	if(!client->cpeData)
		return "vanilla";
	return client->cpeData->appName;
}

bool Client_CheckAuth(CLIENT* client) { //TODO: ClassiCube auth
	return true;
}

void Client_SetPos(CLIENT* client, VECTOR* pos, ANGLE* ang) {
	memcpy(client->playerData->position, pos, sizeof(struct vector));
	memcpy(client->playerData->angle, ang, sizeof(struct angle));
}

void Client_Destroy(CLIENT* client) {
	clients[client->id] = NULL;
	free(client->rdbuf);
	free(client->wrbuf);

	if(client->thread)
		Thread_Close(client->thread);

	if(client->playerData) {
		free(client->playerData->name);
		free(client->playerData->key);
		free(client->playerData);
	}
	if(client->cpeData) {
		EXT* ptr = client->cpeData->firstExtension;
		while(ptr) {
			free(ptr->name);
			free(ptr);
			ptr = ptr->next;
		}
		free(client->cpeData);
	}
}

int Client_Send(CLIENT* client, int len) {
	if(client == Broadcast) {
		for(int i = 0; i < 128; i++) {
			CLIENT* bClient = clients[i];

			if(bClient)
				send(bClient->sock, Broadcast->wrbuf, len, 0);
		}
		return len;
	}

	return send(client->sock, client->wrbuf, len, 0) == len;
}

bool Client_Spawn(CLIENT* client) {
	if(client->playerData->spawned)
		return false;

	for(int i = 0; i < 128; i++) {
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

bool Client_Despawn(CLIENT* client) {
	if(!client->playerData->spawned)
		return false;

	Packet_WriteDespawn(Broadcast, client);
	client->playerData->spawned = false;
	Event_OnDespawn(client);
	return true;
}

bool Client_SendMap(CLIENT* client) {
	if(!client->playerData->currentWorld)
		return false;
	if(client->playerData->mapThread)
		return false;

	Packet_WriteLvlInit(client);
	client->playerData->state = STATE_MOTD;
	client->playerData->mapThread = Thread_Create(&Client_MapThreadProc, client);
	return true;
}

void Client_Disconnect(CLIENT* client) {
	Client_Despawn(client);
	client->status = CLIENT_WAITCLOSE;
	shutdown(client->sock, SD_SEND);
}

void Client_Kick(CLIENT* client, const char* reason) {
	Packet_WriteKick(client, reason);
	Client_Disconnect(client);
}

void Client_UpdatePositions(CLIENT* client) {
	if(!client->playerData->positionUpdated)
		return;
	client->playerData->positionUpdated = false;

	for(int i = 0; i < 128; i++) {
		CLIENT* other = clients[i];
		if(!other || !other->playerData || other->playerData->state != STATE_INGAME || client == other) continue;
		if(client->playerData->currentWorld != other->playerData->currentWorld) continue;
		Packet_WritePosAndOrient(other, client);
	}
}

void Client_Tick(CLIENT* client) {
	if(client->status == CLIENT_AFTERCLOSE) {
		Client_Disconnect(client);
		Client_Destroy(client);
		return;
	}

	if(!client->playerData) return;
	switch (client->playerData->state) {
		case STATE_WLOADDONE:
			Thread_Close(client->playerData->mapThread);
			client->playerData->state = STATE_INGAME;
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
	char* data = client->rdbuf; ++data;
	uchar id = client->rdbuf[0];
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
