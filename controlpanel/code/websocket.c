#include "core.h"

#include "websocket.h"

void WebSocket_Setup(WSFRAME ws, SOCKET fd) {
	ws->state = WS_ST_HDR;
	ws->ready = true;
	ws->_dneed = 2;
	ws->sock = fd;
}

bool WebSocket_ReceiveFrame(WSFRAME ws) {
	if(!ws->ready) return false;

	if(ws->state == WS_ST_DONE) {
		ws->state = WS_ST_HDR;
		ws->_dneed = 2;
	}

	if(ws->state == WS_ST_HDR) {
		uint32_t len = Socket_Receive(ws->sock, ws->hdr, ws->_dneed, 0);

		if(len == ws->_dneed) {
			char plen = *(ws->hdr + 1) & 0x7F;

			if(plen == 126) {
				ws->state = WS_ST_PLEN;
				ws->_dneed = 2;
			} else if(plen == 127) {
				ws->state = WS_ST_PLEN;
				ws->_dneed = 4;
			} else if(plen < 126) {
				ws->state = WS_ST_MASK;
				ws->payload_len = plen;
				ws->_dneed = 4;
			}

			ws->fin = (*ws->hdr >> 0x07) & 1;
			ws->masked = (*(ws->hdr + 1) >> 0x07) & 1;
			ws->opcode = *ws->hdr & 0x0F;
			ws->_dneed = 4;
		}
	}

	if(ws->state == WS_ST_PLEN) {
		uint32_t len = Socket_Receive(ws->sock, (char*)&ws->payload_len, ws->_dneed, 0);

		if(len == ws->_dneed) {
			if(ws->_dneed == 2) {
				ws->payload_len = ntohs((uint16_t)ws->payload_len);
			} else {
				ws->payload_len = ntohl(ws->payload_len);
			}
			ws->state = WS_ST_MASK;
			ws->_dneed = 4;
		}
	}

	if(ws->state == WS_ST_MASK) {
		uint32_t len = Socket_Receive(ws->sock, ws->mask, ws->_dneed, 0);

		if(len == ws->_dneed) {
			ws->state = WS_ST_RECVPL;
			ws->_dneed = ws->payload_len;
		}
	}

	if(ws->state == WS_ST_RECVPL) {
		if(ws->_maxlen < ws->_dneed) {
			Memory_Free(ws->payload);
			ws->_maxlen = ws->_dneed;
			ws->payload = Memory_Alloc(1, ws->_dneed + 1);
		} else
			Memory_Fill(ws->payload, ws->_maxlen, 0);

		uint32_t len = Socket_Receive(ws->sock, ws->payload, ws->_dneed, 0);

		if(len == ws->_dneed) {
			for(uint32_t i = 0; i < len; i++) {
				ws->payload[i] = ws->payload[i] ^ ws->mask[i % 4];
			}

			ws->state = WS_ST_DONE;
			return true;
		}
	}

	return false;
}

void WebSocket_FreeFrame(WSFRAME ws) {
	if(ws->payload) Memory_Free(ws->payload);
	Memory_Free(ws);
}

uint32_t WebSocket_Encode(char* buf, uint32_t len, const char* data, uint32_t dlen, char opcode) {
	uint32_t outlen = dlen + 2;
	if(len < outlen) return 0;

	*buf = 0x80 | (opcode & 0x0F);

	if(dlen < 126) {
		*++buf = (char)dlen;
	} else if(len < 65535) {
		outlen += 2;
		*++buf = 126;
		*(uint16_t*)++buf = htons((uint16_t)dlen); ++buf;
	} else {
		outlen += 4;
		*++buf = 127;
		*(uint32_t*)++buf = htonl(dlen); buf += 3;
	}

	if(len < outlen) return 0;

	Memory_Copy(++buf, data, dlen);
	return outlen;
}
