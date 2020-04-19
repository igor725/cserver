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

typedef struct {
	Socket sock;
	cs_int32 state;
	cs_int32 error;
	char *recvbuf;
	char header[2];
	char mask[4];
	cs_uint16 plen;
	cs_byte opcode;
	cs_bool done;
} WsClient;

API cs_bool WsClient_DoHandshake(WsClient *ws);
API cs_bool WsClient_ReceiveFrame(WsClient *ws);
API cs_bool WsClient_SendFrame(WsClient *ws, cs_byte opcode, const char *buf, cs_uint16 len);
#endif // WEBSOCKET_H
