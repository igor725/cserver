#ifndef WEBSOCKTYPES_H
#define WEBSOCKTYPES_H
#include "core.h"
#include "types/platform.h"

enum _WebSockState {
	WS_ST_HDR,
	WS_ST_PLEN,
	WS_ST_MASK,
	WS_ST_RECVPL,
	WS_ST_DONE
};

enum _WebSockErrors {
	WS_ERR_SUCC,
	WS_ERR_UNKNOWN,
	WS_ERR_MASK,
	WS_ERR_PAYLOAD_TOO_BIG,
	WS_ERR_PAYLOAD_LEN_MISMATCH,
};

typedef struct _WebSock {
	enum _WebSockState state;
	enum _WebSockErrors error;
	cs_byte opcode;
	cs_bool done;
	cs_uint16 plen;
	Socket sock;
	cs_str proto;
	cs_char *recvbuf,
	header[2],
	mask[4];
} WebSock;
#endif
