#include "core.h"

#include "websocket.h"

void WebSocket_Setup(WSFRAME* ws, SOCKET fd) {
	ws->state = WS_ST_HDR;
	ws->ready = true;
	ws->_dneed = 2;
	ws->sock = fd;
}

int WebSocket_ReceiveFrame(WSFRAME* ws) {
	if(!ws->ready) return -1;

	if(ws->state == WS_ST_DONE) {
		ws->state = WS_ST_HDR;
		ws->payload_len = 0;
		Memory_Free(ws->payload);
		ws->payload = NULL;
		ws->_dneed = 2;
	}

	if(ws->state == WS_ST_HDR) {
		int len = recv(ws->sock, ws->hdr, ws->_dneed, 0);

		if(len == ws->_dneed) {
			char len = *ws->hdr & 0x7F;

			if(len == 126) {
				ws->state = WS_ST_PLEN;
				ws->_dneed = 2;
			} else if(len == 127) {
				ws->state = WS_ST_PLEN;
				ws->_dneed = 4;
			} else if(len < 126) {
				ws->state = WS_ST_MASK;
				ws->payload_len = len;
				ws->_dneed = 4;
			}

			ws->fin = (*ws->hdr >> 0x07) & 1;
			ws->masked = (*(ws->hdr + 1) >> 0x07) & 1;
			ws->opcode = *ws->hdr & 0x0F;
			ws->_dneed = 4;
		}
	}

	if(ws->state == WS_ST_PLEN) {
		int len = recv(ws->sock, (char*)&ws->payload_len, ws->_dneed, 0);

		if(len == ws->_dneed) {
			if(ws->_dneed == 2) {
				ws->payload_len = ntohs((ushort)ws->payload_len);
			} else {
				ws->payload_len = ntohl(ws->payload_len);
			}
			ws->state = WS_ST_MASK;
			ws->_dneed = 4;
		}
	}

	if(ws->state == WS_ST_MASK) {
		int len = recv(ws->sock, ws->mask, ws->_dneed, 0);

		if(len == ws->_dneed) {
			ws->state = WS_ST_RECVPL;
			ws->_dneed = ws->payload_len;
		}
	}

	if(ws->state == WS_ST_RECVPL) {

	}

	return -1;
}

bool WebSocket_Encode(char* buf, size_t len, const char* data, size_t dlen, char opcode) {
	if(len + 2 < dlen) return false;

	*buf = 0x80 | (opcode & 0x0F);

	if(dlen < 126) {
		*++buf = dlen;
	} else if(len < 65535) {
		*(ushort*)++buf = htons(dlen); ++buf;
	} else {
		*(uint*)++buf = htonl(dlen); buf += 3;
	}

	Memory_Copy(buf, data, dlen);
	return true;
}
