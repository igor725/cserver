#ifndef WEBSOCKTYPES_H
#define WEBSOCKTYPES_H
#include "core.h"
#include "types/platform.h"

typedef enum _EWebSockState {
	WS_STATE_HDR,
	WS_STATE_PLEN,
	WS_STATE_MASK,
	WS_STATE_RECVPL,
	WS_STATE_DONE
} EWebSockState;

typedef enum _EWebSockErrors {
	WS_ERROR_SUCC,
	WS_ERROR_SOCKET,
	WS_ERROR_CONTINUE,
	WS_ERROR_MASK,
	WS_ERROR_PAYLOAD_TOO_BIG,
	WS_ERROR_PAYLOAD_LEN_MISMATCH,
} EWebSockErrors;

typedef struct _WebSock {
	EWebSockState state;
	EWebSockErrors error;
	cs_byte opcode;
	cs_bool done;
	cs_uint16 plen;
	Socket sock;
	cs_str proto;
	cs_char *recvbuf;
	cs_char header[2];
	cs_char mask[4];
} WebSock;
#endif
