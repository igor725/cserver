#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include "core.h"
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

void Client_SetPosition(CLIENT* self, VECTOR* pos, ANGLE* ang) {
	Packet_WritePosAndOrient(self, NULL);
}

void Client_Destroy(CLIENT* self) {
	free(self->rdbuf);
	free(self->wrbuf);

	if(self->playerData != NULL) {
		free(self->playerData->position);
		free(self->playerData->angle);
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

int Client_Send(CLIENT* self, uint len) {
	return send(self->sock, self->wrbuf, len, 0);
}

bool Client_SendMap(CLIENT* self) {
	if(self->playerData->currentWorld == NULL)
		return false;

	return true;
}

void Client_Disconnect(CLIENT* self) {
	self->state = CLIENT_WAITCLOSE;
	shutdown(self->sock, SD_SEND);
}

int Client_ThreadProc(void* lpParam) {
	CLIENT* self = (CLIENT*)lpParam;

	while(1) {
		if(self->state == CLIENT_WAITCLOSE) {
			int len = recv(self->sock, self->rdbuf, 131, 0);
			if(len <= 0) {
				closesocket(self->sock);
				self->state = CLIENT_AFTERCLOSE;
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
				self->state = CLIENT_AFTERCLOSE;
				break;
			}
		}
	}
	return 0;
}

void AcceptClients() {
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
			tmp->state = CLIENT_OK;
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
