#ifndef CP_WEBSOCKET_H
#define CP_WEBSOCKET_H

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

enum wState {
	WS_ST_HDR,
	WS_ST_PLEN,
	WS_ST_MASK,
	WS_ST_RECVPL,
	WS_ST_DONE
};

typedef struct wsFrame {
	SOCKET sock;
	char hdr[2];
	char state;
	char opcode;
	char mask[4];
	uint32_t payload_len;
	uint32_t _maxlen;
	char* payload;
	uint32_t _dneed;
	uint16_t _drcvd;
	bool fin;
	bool masked;
	bool ready;
} *WSFRAME;

void WebSocket_Setup(WSFRAME ws, SOCKET fd);
bool WebSocket_ReceiveFrame(WSFRAME ws);
void WebSocket_FreeFrame(WSFRAME ws);
uint32_t WebSocket_Encode(char* buf, uint32_t len, const char* data, uint32_t dlen, char opcode);
#endif
