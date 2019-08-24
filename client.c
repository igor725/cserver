#include <windows.h>
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

boolean Client_IsSupportExt(CLIENT* self, const char* packetName) {
	return false;
}

void Client_SendMap(CLIENT* self) {
	if(!self->currentWorld)
		return;
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

		if(self->rdbufpos > 0) {
			short packetSize = Packet_GetSize(*self->rdbuf, self);

			if(packetSize < 1) {
				Packet_WriteKick(self, "Invalid packet ID");
				continue;
			}
			wait = packetSize - self->rdbufpos;
		}

		if(wait == 0) {
			Client_HandlePacket(self);
			self->rdbufpos = 0;
			continue;
		} else if(wait > 0) {
			int len = recv(self->sock, self->rdbuf + self->rdbufpos, wait, 0);

			if(len > 0) {
				self->rdbufpos += len;
			} else {
				self->state = CLIENT_AFTERCLOSE;
				break;
			}
		}

		Sleep(10);
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
		tmp->rdbufpos = 0;
		tmp->wrbufpos = 0;
		tmp->appName = "vanilla";
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
	if(packet == NULL)
		return;

	char* data = ++self->rdbuf;
	if(packet->haveCPEImp == true)
		if(packet->cpeHandler == NULL)
			packet->handler(self, data);
		else
			packet->cpeHandler(self, data);
	else
		if(packet->handler != NULL)
			packet->handler(self, data);
}
