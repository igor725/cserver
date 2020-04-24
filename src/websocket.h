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

typedef struct _WebSock {
	Socket sock;
	cs_char *recvbuf,
	header[2],
	mask[4];
	cs_int32 state,
	error;
	cs_uint16 plen;
	cs_byte opcode;
	cs_bool done;
} WebSock;

cs_bool WebSock_DoHandshake(WebSock *ws);
cs_bool WebSock_ReceiveFrame(WebSock *ws);
cs_bool WebSock_SendFrame(WebSock *ws, cs_byte opcode, const cs_char *buf, cs_uint16 len);
#endif // WEBSOCKET_H
