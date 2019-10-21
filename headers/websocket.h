#ifndef WEBSOCKET_H
#define WEBSOCKET_H

enum wState {
	WS_ST_HDR,
	WS_ST_PLEN,
	WS_ST_MASK,
	WS_ST_RECVPL,
	WS_ST_DONE
};

enum wErrors {
	WS_ERR_SUCC,
	WS_ERR_UNKNOWN,
	WS_ERR_MASK,
	WS_ERR_PAYLOAD_TOO_BIG,
	WS_ERR_PAYLOAD_LEN_MISMATCH,
};

typedef struct wsClient {
	SOCKET sock;
	int state;
	int error;
	char* recvbuf;
	char header[2];
	char mask[4];
	uint16_t plen;
	uint8_t opcode;
	bool done;
} *WSCLIENT;

bool WsClient_DoHandshake(WSCLIENT ws);
bool WsClient_ReceiveFrame(WSCLIENT ws);
bool WsClient_SendHeader(WSCLIENT ws, uint8_t opcode, uint16_t len);
#endif
