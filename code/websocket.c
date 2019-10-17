#include "core.h"
#include "sha1.h"
#include "websocket.h"

static bool sockReadLine(SOCKET sock, char* line, uint32_t len) {
	uint32_t linepos = 0;
	char sym;

	while(linepos < len) {
		if(Socket_Receive(sock, &sym, 1, 0) == 1) {
			if(sym == '\n') {
				line[linepos] = '\0';
				break;
			} else if(sym != '\r')
				line[linepos++] = sym;
		}
	}

	line[linepos] = '\0';
	return true;
}

const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char* SHA1toB64(uint8_t* in, char* out) {
	for (int i = 0, j = 0; i < 20; i += 3, j += 4) {
		int v = in[i];
		v = i + 1 < 20 ? v << 8 | in[i + 1] : v << 8;
		v = i + 2 < 20 ? v << 8 | in[i + 2] : v << 8;

		out[j] = b64chars[(v >> 18) & 0x3F];
		out[j + 1] = b64chars[(v >> 12) & 0x3F];
		if (i + 1 < 20) {
			out[j + 2] = b64chars[(v >> 6) & 0x3F];
		} else {
			out[j + 2] = '=';
		}
		if (i + 2 < 20) {
			out[j + 3] = b64chars[v & 0x3F];
		} else {
			out[j + 3] = '=';
		}
	}

	out[28] = '\0';
	return out;
}

#define WS_RESP "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Protocol: ClassiCube\r\nSec-WebSocket-Accept: %s\r\n\r\n"

bool WsClient_DoHandshake(WSCLIENT ws) {
	char line[4096] = {0}, wskey[32] = {0}, b64[30] = {0};
	uint8_t hash[20] = {0};
	bool haveUpgrade = false;
	int wskeylen = 0;

	sockReadLine(ws->sock, line, 4095); // Skipping request line
	while(sockReadLine(ws->sock, line, 4095)) {
		if(*line == '\0') break;

		char* value = (char*)String_FirstChar(line, ':');
		*value = '\0';value += 2;

		if(String_CaselessCompare(line, "Sec-WebSocket-Key")) {
			wskeylen = (int)String_Copy(wskey, 32, value);
		} else if(String_CaselessCompare(line, "Sec-WebSocket-Version")) {
			if(String_ToInt(value) != 13) break;
		} else if(String_CaselessCompare(line, "Upgrade")) {
			haveUpgrade = String_CaselessCompare(value, "websocket");
		}
	}

	if(haveUpgrade && String_Length(wskey) > 0) {
		SHA1_CTX ctx;
		SHA1_Init(&ctx);
		SHA1_Update(&ctx, (uint8_t*)wskey, wskeylen);
		SHA1_Update(&ctx, (uint8_t*)"258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
		SHA1_Final((uint8_t*)hash, &ctx);
		SHA1toB64(hash, b64);

		String_FormatBuf(line, 4096, WS_RESP, b64);
		Socket_Send(ws->sock, line, (int)String_Length(line));
		return true;
	} else {
		//TODO: HTTP error 4xx
	}

	return false;
}

bool WsClient_ReceiveFrame(WSCLIENT ws) {
	if(ws->state == WS_ST_DONE) ws->state = WS_ST_HDR;

	if(ws->state == WS_ST_HDR) {
		uint32_t len = Socket_Receive(ws->sock, ws->header, 2, 0);

		if(len == 2) {
			char plen = ws->header[1] & 0x7F;

			if((ws->header[1] >> 0x07) & 1) {
				ws->opcode = ws->header[0] & 0x0F;
				ws->done = (ws->header[0] >> 0x07) & 1;
				ws->plen = plen;

				if(plen == 126) {
					ws->state = WS_ST_PLEN;
				} else if(plen < 126) {
					ws->state = WS_ST_MASK;
				} else
					return false;
			} else {
				ws->error = WS_ERR_MASK;
				return false;
			}
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
		if(ws->plen > 0) {
			uint32_t len = Socket_Receive(ws->sock, ws->recvbuf, ws->plen, 0);

			if(len == ws->plen) {
				for(uint32_t i = 0; i < len; i++) {
					ws->recvbuf[i] ^= ws->mask[i % 4];
				}
			} else
				return false;
		}

		ws->state = WS_ST_DONE;
		return true;
	}

	return false;
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
