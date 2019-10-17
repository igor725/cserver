#include "core.h"
#include "websocket.h"

bool WsClient_DoHandshake(WSCLIENT ws) {
	(void)ws;
	/*
		TODO: Работа с HTTP запросом от клиента и
		проверка совместимости протоколов.
	*/

	return false;
}

bool WsClient_ReceiveFrame(WSCLIENT ws) {
	if(ws->state == WS_ST_DONE)
		ws->state = WS_ST_HDR;

	if(ws->state == WS_ST_HDR) {
		uint32_t len = Socket_Receive(ws->sock, ws->header, 2, 0);

		if(len == 2) {
			char plen = *(ws->header + 1) & 0x7F;

			if(plen == 126) {
				ws->state = WS_ST_PLEN;
			} else if(plen < 126) {
				ws->state = WS_ST_MASK;
			} else {
				return false;
			}

			// bool fin = (*ws->hdr >> 0x07) & 1;
			// bool masked = (*(ws->hdr + 1) >> 0x07) & 1;
			ws->opcode = *ws->header & 0x0F;
			ws->plen = plen;
		}
	}

	if(ws->state == WS_ST_PLEN) {
		uint32_t len = Socket_Receive(ws->sock, (char*)&ws->plen, 2, 0);

		if(len == 2) {
			ws->plen = ntohs(ws->plen);
			if(ws->plen > 131) return false;
			ws->state = WS_ST_MASK;
		}
	}

	if(ws->state == WS_ST_MASK) {
		uint32_t len = Socket_Receive(ws->sock, ws->mask, 4, 0);
		if(len == 4) ws->state = WS_ST_RECVPL;
	}

	if(ws->state == WS_ST_RECVPL) {
		uint32_t len = Socket_Receive(ws->sock, ws->recvbuf, ws->plen, 0);

		if(len == ws->plen) {
			ws->state = WS_ST_DONE;
			for(uint32_t i = 0; i < len; i++) {
				ws->recvbuf[i] = ws->recvbuf[i] ^ ws->mask[i % 4];
			}
			return true;
		}
	}

	return false;
}

void WsClient_Free(WSCLIENT ws) {
	if(ws->recvbuf) Memory_Free(ws->recvbuf);
	Memory_Free(ws);
}

bool WsClient_SendHeader(WSCLIENT ws, uint8_t opcode, uint16_t len) {
	uint16_t hdrlen = 2;
	char hdr[4] = {0};

	hdr[0] = 0x80 | (opcode & 0x0F);

	if(len < 126) {
		hdr[1] = (char)len;
	} else if(len < 65535) {
		hdrlen = 4;
		hdr[1] = 126;
		*(uint16_t*)&hdr[2] = htons((uint16_t)len);
	} else
		return false;

	return Socket_Send(ws->sock, hdr, hdrlen) == hdrlen;
}
