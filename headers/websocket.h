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
	Socket sock;
	int32_t state;
	int32_t error;
	char* recvbuf;
	char header[2];
	char mask[4];
	uint16_t plen;
	uint8_t opcode;
	bool done;
} *WsClient;

bool WsClient_DoHandshake(WsClient ws);
bool WsClient_ReceiveFrame(WsClient ws);
bool WsClient_SendHeader(WsClient ws, uint8_t opcode, uint16_t len);
#endif
