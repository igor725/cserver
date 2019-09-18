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

typedef struct ws_frame {
	SOCKET sock;
	char hdr[2];
	char state;
	char opcode;
	char mask[4];
	uint payload_len;
	char* payload;
	ushort _dneed;
	ushort _drcvd;
	bool fin;
	bool masked;
	bool ready;
} WSFRAME;
#endif
