#include "core.h"
#include "str.h"
#include "platform.h"
#include "netbuffer.h"
#include "websock.h"
#include "strstor.h"
#include "hash.h"

cs_str ws_resp =
"HTTP/1.1 101 Switching Protocols\r\n"
"Connection: Upgrade\r\n"
"Upgrade: websocket\r\n"
"Sec-WebSocket-Protocol: %s\r\n"
"Sec-WebSocket-Accept: %s\r\n\r\n";

cs_str ws_err =
"HTTP/1.1 %d %s\r\n"
"Connection: Close\r\n"
"Content-Type: text/plain\r\n"
"Content-Length: %d\r\n\r\n";

INL static void ProcessHandshake(WebSock *ws, NetBuffer *nb) {
	cs_int32 ret;

	while(true) {
		switch(NetBuffer_ReadLine(nb, ws->shake.line, 1024)) {
			case -1:
				ws->error = WEBSOCK_ERROR_STRLEN;
				return;
			case -2:
				ws->error = WEBSOCK_ERROR_CONTINUE;
				return;
			
			default: break;
		}

		if(ws->shake.state == WEBSHAKE_STATE_HTTP) {
			cs_str httpver = String_LastChar(ws->shake.line, 'H');
			if(!httpver || !String_CaselessCompare(httpver, "HTTP/1.1")) {
				cs_char *buffer = NetBuffer_StartWrite(nb, 1024);
				ret = String_FormatBuf(buffer, 1024,
					ws_err, 505, "HTTP Version Not Supported", 0
				);
				NetBuffer_EndWrite(nb, ret);
				ws->error = WEBSOCK_ERROR_HTTPVER;
				return;
			}

			ws->shake.state = WEBSHAKE_STATE_HEADERS;
		} else if(ws->shake.state == WEBSHAKE_STATE_HEADERS) {
			if(*ws->shake.line == '\0') {
				ws->shake.state = WEBSHAKE_STATE_FINISHING;
				break;
			}

			if(String_CaselessCompare2(ws->shake.line, "Sec-WebSocket-Key: ", 19)) {
				ws->shake.keylen = (cs_int32)String_Copy(ws->shake.key, 32, ws->shake.line + 19);
				if(ws->shake.keylen <= 0) {
					ws->error = WEBSOCK_ERROR_INVKEY;
					return;
				}
				ws->shake.hdrflags |= WEBSHAKE_FLAG_KEYOK;
			} else if(String_CaselessCompare2(ws->shake.line, "Sec-WebSocket-Version: ", 23)) {
				if(String_ToInt(ws->shake.line + 23) != 13) {
					ws->error = WEBSOCK_ERROR_PROTOVER;
					return;
				}
				ws->shake.hdrflags |= WEBSHAKE_FLAG_VEROK;
			} else if(String_CaselessCompare2(ws->shake.line, "Upgrade: ", 9)) {
				if(!String_CaselessCompare(ws->shake.line + 9, "websocket")) {
					ws->error = WEBSOCK_ERROR_NOTWS;
					return;
				}
				ws->shake.hdrflags |= WEBSHAKE_FLAG_UPGOK;
			}
		}
	}

	if(ws->shake.state == WEBSHAKE_STATE_FINISHING) {
		if(ws->shake.hdrflags == WEBSHAKE_FLAGS_OK) {
			SHA_CTX ctx;
			cs_char b64[30];
			cs_byte hash[20];
			cs_char *buffer = NetBuffer_StartWrite(nb, 1024);

			if(SHA1_Init(&ctx)) {
				SHA1_Update(&ctx, ws->shake.key, ws->shake.keylen);
				SHA1_Update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
				SHA1_Final(hash, &ctx);

				ws->shake.state = WEBSHAKE_STATE_DONE;
				ws->state = WEBSOCK_STATE_HEADER;
				String_ToB64(hash, 20, b64);
				ret = String_FormatBuf(buffer, 1024, ws_resp, ws->proto, b64);
			} else {
				cs_str msg = Sstor_Get("WS_SHAERR");
				cs_int32 msglen = (cs_int32)String_Length(msg);
				ret = String_FormatBuf(buffer, 1024, ws_err, 500, "Internal Server Error", msglen);
				ws->error = WEBSOCK_ERROR_HASHINIT;
			}

			NetBuffer_EndWrite(nb, ret);
			nb->wsupgrade = true;
		} else {
			ws->error = WEBSOCK_ERROR_NOTWS;
			return;
		}
	}

	if(ws->error == WEBSOCK_ERROR_SUCC)
		ws->error = WEBSOCK_ERROR_CONTINUE;
}

cs_bool WebSock_Tick(WebSock *ws, NetBuffer *nb) {
	if(ws->state == WEBSOCK_STATE_HANDSHAKE) {
		ProcessHandshake(ws, nb);
		return false; // Во время хендшейка не может быть получен фрейм
	}

	if(ws->state == WEBSOCK_STATE_DONE)
		ws->state = WEBSOCK_STATE_HEADER;

	if(ws->state == WEBSOCK_STATE_HEADER) {
		if(NetBuffer_AvailRead(nb) < 2) {
			ws->error = WEBSOCK_ERROR_CONTINUE;
			return false;
		}

		cs_char *header = NetBuffer_Read(nb, 2);

		if(header[1] & 0x80) {
			ws->opcode = header[0] & 0x0F;
			ws->done = (header[0] >> 0x07) & 0x01;
			ws->paylen = header[1] & 0x7F;

			if(ws->paylen == 126) {
				ws->state = WEBSOCK_STATE_LENGTH;
			} else if(ws->paylen < 126) {
				ws->state = WEBSOCK_STATE_MASK;
			} else {
				ws->error = WEBSOCK_ERROR_PAYLOAD_TOO_BIG;
				return false;
			}
		}
	}

	if(ws->state == WEBSOCK_STATE_LENGTH) {
		if(NetBuffer_AvailRead(nb) < 2) {
			ws->error = WEBSOCK_ERROR_CONTINUE;
			return false;
		}

		cs_char *paylength = NetBuffer_Read(nb, 2);
		ws->paylen = ntohs(*(cs_uint16 *)paylength);

		if(ws->paylen > ws->maxpaylen) {
			ws->error = WEBSOCK_ERROR_PAYLOAD_TOO_BIG;
			return false;
		}

		ws->state = WEBSOCK_STATE_MASK;
	}

	if(ws->state == WEBSOCK_STATE_MASK) {
		if(NetBuffer_AvailRead(nb) < 4) {
			ws->error = WEBSOCK_ERROR_CONTINUE;
			return false;
		}

		cs_char *mask = NetBuffer_Read(nb, 4);
		ws->state = WEBSOCK_STATE_PAYLD;
		for(cs_int32 i = 0; i < 4; i++)
			ws->mask[i] = mask[i];
	}

	if(ws->state == WEBSOCK_STATE_PAYLD) {
		if(NetBuffer_AvailRead(nb) < ws->paylen) {
			ws->error = WEBSOCK_ERROR_CONTINUE;
			return false;
		}

		cs_char *payload = NetBuffer_Read(nb, ws->paylen);
		if(payload) {
			ws->payload = payload;
			ws->state = WEBSOCK_STATE_DONE;
			for(cs_uint16 i = 0; i < ws->paylen; i++)
				payload[i] ^= ws->mask[i % 4];

			return true;
		}
	}

	return false;
}

cs_int32 WebSock_WriteHeader(Socket sock, cs_byte opcode, cs_uint32 len, cs_int32 *hdrlen) {
	*hdrlen = 2;
	cs_char hdr[4] = {0, 0, 0, 0};
	hdr[0] = 0x80 | opcode;

	if(len < 126)
		hdr[1] = (cs_char)len;
	else if(len < 65536) {
		*hdrlen = 4;
		hdr[1] = 126;
		*(cs_uint16 *)&hdr[2] = htons((cs_uint16)len);
	} else {
		*hdrlen = -1;
		return 0;
	}

	return Socket_Send(sock, hdr, *hdrlen);
}

EWebSockErrors WebSock_GetErrorCode(WebSock *ws) {
	return ws->error;
}

cs_str WebSock_GetError(WebSock *ws) {
	switch(ws->error) {
		case WEBSOCK_ERROR_SUCC: return "WEBSOCK_ERROR_SUCC";
		case WEBSOCK_ERROR_SOCKET: return "WEBSOCK_ERROR_SOCKET";
		case WEBSOCK_ERROR_STRLEN: return "WEBSOCK_ERROR_STRLEN";
		case WEBSOCK_ERROR_CONTINUE: return "WEBSOCK_ERROR_CONTINUE";
		case WEBSOCK_ERROR_HTTPVER: return "WEBSOCK_ERROR_HTTPVER";
		case WEBSOCK_ERROR_INVKEY: return "WEBSOCK_ERROR_INVKEY";
		case WEBSOCK_ERROR_PROTOVER: return "WEBSOCK_ERROR_PROTOVER";
		case WEBSOCK_ERROR_NOTWS: return "WEBSOCK_ERROR_NOTWS";
		case WEBSOCK_ERROR_HASHINIT: return "WEBSOCK_ERROR_HASHINIT";
		case WEBSOCK_ERROR_HEADER: return "WEBSOCK_ERROR_HEADER";
		case WEBSOCK_ERROR_MASK: return "WEBSOCK_ERROR_MASK";
		case WEBSOCK_ERROR_PAYLOAD_TOO_BIG: return "WEBSOCK_ERROR_PAYLOAD_TOO_BIG";
	}

	return "WEBSOCK_ERROR_UNKNOWN";
}
