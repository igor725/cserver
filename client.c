#include "winsock2.h"
#include "core.h"
#include "string.h"
#include "zlib.h"
#include "world.h"
#include "client.h"
#include "packets.h"
#include "server.h"

void* listenThread;

void Client_Listen() {
	listenThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)&AcceptClients_ThreadProc,
		NULL,
		0,
		NULL
	);
}

void Client_StopListen() {
	if(listenThread)
		CloseHandle(listenThread);
}

int Client_FindFreeID() {
	for(int i = 0; i < 127; i++) {
		if(clients[i] == NULL)
			return i;
	}
	return -1;
}

bool Client_IsSupportExt(CLIENT* client, const char* extName) {
	if(client->cpeData == NULL)
		return false;

	EXT* ptr = client->cpeData->firstExtension;
	while(ptr != NULL) {
		if(stricmp(ptr->name, extName) == 0) {
			return true;
		}
		ptr = ptr->next;
	}
	return false;
}

char* Client_GetAppName(CLIENT* client) {
	if(client->cpeData == NULL)
		return "vanilla";
	return client->cpeData->appName;
}

bool Client_CheckAuth(CLIENT* client) { //TODO: ClassiCube auth
	return true;
}

void Client_SetPos(CLIENT* client, VECTOR* pos, ANGLE* ang) {
	memcpy(&client->playerData->position, pos, sizeof(struct vector));
	memcpy(&client->playerData->angle, ang, sizeof(struct angle));
}

void Client_Destroy(CLIENT* client) {
	clients[client->id] = NULL;
	free(client->rdbuf);
	free(client->wrbuf);

	if(client->thread != NULL)
		CloseHandle(client->thread);

	if(client->playerData != NULL) {
		free(client->playerData->name);
		free(client->playerData->key);
		free(client->playerData);
	}
	if(client->cpeData != NULL) {
		EXT* ptr = client->cpeData->firstExtension;
		while(ptr != NULL) {
			free(ptr->name);
			free(ptr);
			ptr = ptr->next;
		}
		free(client->cpeData);
	}
}

int Client_Send(CLIENT* client, int len) {
	return send(client->sock, client->wrbuf, len, 0) == len;
}

int Client_MapThreadProc(void* lpParam) {
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
			deflateEnd(&stream);
			return 0;
		}
	} while(stream.avail_out == 0);

	deflateEnd(&stream);
	Packet_WriteLvlFin(client);
	client->playerData->state = STATE_WLOADDONE;
	for(int i = 0; i < 128; i++) {
		CLIENT* other = clients[i];
		if(other == NULL) continue;

		Packet_WriteSpawn(other, client);
		if(client != other)
			Packet_WriteSpawn(client, other);
	}
	return 0;
}

bool Client_SendMap(CLIENT* client) {
	if(client->playerData->currentWorld == NULL)
		return false;
	if(client->playerData->mapThread != NULL)
		return false;

	Packet_WriteLvlInit(client);
	client->playerData->state = STATE_WLOAD;
	client->playerData->mapThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)&Client_MapThreadProc,
		client,
		0,
		NULL
	);

	return true;
}

void Client_Disconnect(CLIENT* client) {
	client->status = CLIENT_WAITCLOSE;
	shutdown(client->sock, SD_SEND);
}

int Client_ThreadProc(void* lpParam) {
	CLIENT* client = (CLIENT*)lpParam;

	while(1) {
		if(client->status == CLIENT_WAITCLOSE) {
			int len = recv(client->sock, client->rdbuf, 131, 0);
			if(len <= 0) {
				client->status = CLIENT_AFTERCLOSE;
				closesocket(client->sock);
				break;
			}
			continue;
		}
		ushort wait = 1;

		if(client->bufpos > 0) {
			short packetSize = Packet_GetSize(*client->rdbuf, client);

			if(packetSize < 1) {
				Packet_WriteKick(client, "Invalid packet ID");
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

void Client_Tick(CLIENT* client) {
	if(client->status == CLIENT_AFTERCLOSE) {
		Client_Destroy(client);
		return;
	}

	if(client->playerData == NULL) return;
	switch (client->playerData->state) {
		case STATE_WLOADDONE:
			CloseHandle(client->playerData->mapThread);
			break;
		case STATE_WLOADERR:
			Packet_WriteKick(client, "Map loading error");
			break;
	}
}

void AcceptClients() {
	struct sockaddr_in caddr;
	int caddrsz = sizeof caddr;

	SOCKET fd = accept(server, (struct sockaddr*)&caddr, &caddrsz);
	if(fd != INVALID_SOCKET) {
	 	CLIENT* tmp = malloc(sizeof(struct client));
		memset(tmp, 0, sizeof(struct client));

		tmp->sock = fd;
		tmp->bufpos = 0;
		tmp->rdbuf = malloc(131);
		tmp->wrbuf = malloc(2048);

		int id = Client_FindFreeID();
		if(id >= 0) {
			tmp->id = id;
			tmp->status = CLIENT_OK;
			tmp->thread = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)&Client_ThreadProc,
				tmp,
				0,
				NULL
			);
			clients[id] = tmp;
		} else {
			Packet_WriteKick(tmp, "Server is full");
		}
	}
}

int AcceptClients_ThreadProc(void* lpParam) {
	while(1)AcceptClients();
	return 0;
}

void Client_HandlePacket(CLIENT* client) {
	uchar id = client->rdbuf[0];
	PACKET* packet = packets[id];
	if(packet == NULL) return;

	bool ret = false;
	char* data = client->rdbuf; ++data;

	if(packet->haveCPEImp == true)
		if(packet->cpeHandler == NULL)
			ret = packet->handler(client, data);
		else
			ret = packet->cpeHandler(client, data);
	else
		if(packet->handler != NULL)
			ret = packet->handler(client, data);

	if(ret == false)
		Packet_WriteKick(client, "Packet reading error");
}
