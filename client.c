#include "core.h"
#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <zlib.h>
#include "world.h"
#include "client.h"
#include "packets.h"
#include "server.h"

void Client_InitListen() {
	CLIENT* clients[256] = {0};
	void* listenThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)&AcceptClients_ThreadProc,
		NULL,
		0,
		NULL
	);
}

int Client_FindFreeID() {
	for(int i = 0; i < 127; i++) {
		if(clients[i] == NULL)
			return i;
	}
	return -1;
}

bool Client_IsSupportExt(CLIENT* self, const char* extName) {
	if(self->cpeData == NULL)
		return false;

	EXT* ptr = self->cpeData->firstExtension;
	while(ptr != NULL) {
		if(stricmp(ptr->name, extName) == 0) {
			return true;
		}
		ptr = ptr->next;
	}
	return false;
}

char* Client_GetAppName(CLIENT* self) {
	if(self->cpeData == NULL)
		return "vanilla";
	return self->cpeData->appName;
}

bool Client_CheckAuth(CLIENT* self) { //TODO: ClassiCube auth
	return true;
}

void Client_SetPos(CLIENT* self, VECTOR* pos, ANGLE* ang) {
	memcpy(&self->playerData->position, pos, sizeof(struct vector));
	memcpy(&self->playerData->angle, ang, sizeof(struct angle));
}

void Client_Destroy(CLIENT* self) {
	free(self->rdbuf);
	free(self->wrbuf);

	if(self->thread != NULL)
		CloseHandle(self->thread);

	if(self->playerData != NULL) {
		free(self->playerData->name);
		free(self->playerData->key);
		free(self->playerData);
	}
	if(self->cpeData != NULL) {
		EXT* ptr = self->cpeData->firstExtension;
		while(ptr != NULL) {
			free(ptr->name);
			free(ptr);
			ptr = ptr->next;
		}
		free(self->cpeData);
	}
}

int Client_Send(CLIENT* self, int len) {
	return send(self->sock, self->wrbuf, len, 0) == len;
}

int Client_MapThreadProc(void* lpParam) {
	CLIENT* self = (CLIENT*)lpParam;
	WORLD* world = self->playerData->currentWorld;

	z_stream stream = {0};

	self->wrbuf[0] = 0x03;
	self->wrbuf[1027] = 0;
	ushort* len = (ushort*)(self->wrbuf + 1);
	char* out = (char*)len + 2;
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
		Log_Error("zlib deflateInit: %s", zError(ret));
		return 0;
	}

	stream.avail_in = world->size;
	stream.next_in = world->data;

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			Log_Error("zlib deflate: %s", zError(ret));
			deflateEnd(&stream);
			return 0;
		}

		*len = htons(1024 - stream.avail_out);
		if(!Client_Send(self, 1028)) {
			Log_Error("Client disconnected while map send in progress:(");
			deflateEnd(&stream);
			return 0;
		}
	} while(stream.avail_out == 0);

	deflateEnd(&stream);
	Packet_WriteLvlFin(self);
	self->playerData->state = STATE_INGAME;
	for(int i = 0; i < 128; i++) {
		CLIENT* other = clients[i];
		if(other == NULL) continue;

		Packet_WriteSpawn(other, self);
		if(self != other)
			Packet_WriteSpawn(self, other);
	}
	return 0;
}

bool Client_SendMap(CLIENT* self) {
	if(self->playerData->currentWorld == NULL)
		return false;
	if(self->playerData->mapThread != NULL)
		return false;

	Packet_WriteLvlInit(self);
	self->playerData->state = STATE_WLOAD;
	self->playerData->mapThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)&Client_MapThreadProc,
		self,
		0,
		NULL
	);

	return true;
}

void Client_Disconnect(CLIENT* self) {
	self->status = CLIENT_WAITCLOSE;
	shutdown(self->sock, SD_SEND);
}

int Client_ThreadProc(void* lpParam) {
	CLIENT* self = (CLIENT*)lpParam;

	while(1) {
		if(self->status == CLIENT_WAITCLOSE) {
			int len = recv(self->sock, self->rdbuf, 131, 0);
			if(len <= 0) {
				self->status = CLIENT_AFTERCLOSE;
				closesocket(self->sock);
				break;
			}
			continue;
		}
		ushort wait = 1;

		if(self->bufpos > 0) {
			short packetSize = Packet_GetSize(*self->rdbuf, self);

			if(packetSize < 1) {
				Packet_WriteKick(self, "Invalid packet ID");
				continue;
			}
			wait = packetSize - self->bufpos;
		}

		if(wait == 0) {
			Client_HandlePacket(self);
			self->bufpos = 0;
			continue;
		} else if(wait > 0) {
			int len = recv(self->sock, self->rdbuf + self->bufpos, wait, 0);

			if(len > 0) {
				self->bufpos += len;
			} else {
				self->status = CLIENT_AFTERCLOSE;
				break;
			}
		}
	}

	return 0;
}

static void AcceptClients() {
	static struct sockaddr_in caddr;
	static int caddrsz = sizeof caddr;

	SOCKET fd = accept(server, (struct sockaddr*)&caddr, &caddrsz);
	if(fd != INVALID_SOCKET) {
	 	CLIENT* tmp = (CLIENT*)malloc(sizeof(struct client));
		memset(tmp, 0, sizeof(struct client));

		tmp->sock = fd;
		tmp->bufpos = 0;
		tmp->rdbuf = (char*)malloc(131);
		tmp->wrbuf = (char*)malloc(2048);

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

void Client_HandlePacket(CLIENT* self) {
	uchar id = self->rdbuf[0];
	PACKET* packet = packets[id];
	if(packet == NULL) return;

	bool ret;
	char* data = self->rdbuf; ++data;

	if(packet->haveCPEImp == true)
		if(packet->cpeHandler == NULL)
			ret = packet->handler(self, data);
		else
			ret = packet->cpeHandler(self, data);
	else
		if(packet->handler != NULL)
			ret = packet->handler(self, data);

	if(ret == false)
		Packet_WriteKick(self, "Packet reading error");
}
