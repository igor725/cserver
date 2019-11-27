#ifndef WEBSOCKET_H
#define WEBSOCKET_H
enum {
	WS_ST_HDR,
	WS_ST_PLEN,
	WS_ST_MASK,
	WS_ST_RECVPL,
	WS_ST_DONE
};

enum {
	WS_ERR_SUCC,
	WS_ERR_UNKNOWN,
	WS_ERR_MASK,
	WS_ERR_PAYLOAD_TOO_BIG,
	WS_ERR_PAYLOAD_LEN_MISMATCH,
};

typedef struct wsClient {
	Socket sock;
	cs_int32 state;
	cs_int32 error;
	char* recvbuf;
	char header[2];
	char mask[4];
	cs_uint16 plen;
	cs_uint8 opcode;
	cs_bool done;
} *WsClient;

cs_bool WsClient_DoHandshake(WsClient ws);
cs_bool WsClient_ReceiveFrame(WsClient ws);
cs_bool WsClient_SendHeader(WsClient ws, cs_uint8 opcode, cs_uint16 len);
#endif // WEBSOCKET_H
