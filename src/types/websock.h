#ifndef WEBSOCKTYPES_H
#define WEBSOCKTYPES_H
#include "core.h"
#include "types/platform.h"

#define WEBSHAKE_FLAG_UPGOK BIT(0)
#define WEBSHAKE_FLAG_VEROK BIT(1)
#define WEBSHAKE_FLAG_KEYOK BIT(2)
#define WEBSHAKE_FLAGS_OK (WEBSHAKE_FLAG_UPGOK | WEBSHAKE_FLAG_VEROK | WEBSHAKE_FLAG_KEYOK)
#define WEBSOCK_FRAME_MAXSIZE 65535

typedef enum _EWebShakeState {
	WEBSHAKE_STATE_HTTP,
	WEBSHAKE_STATE_HEADERS,
	WEBSHAKE_STATE_FINISHING,
	WEBSHAKE_STATE_DONE
} EWebShakeState;

typedef enum _EWebSockState {
	WEBSOCK_STATE_HANDSHAKE,
	WEBSOCK_STATE_HEADER,
	WEBSOCK_STATE_LENGTH,
	WEBSOCK_STATE_MASK,
	WEBSOCK_STATE_PAYLD,
	WEBSOCK_STATE_DONE
} EWebSockState;

typedef enum _EWebSockErrors {
	WEBSOCK_ERROR_SUCC,
	WEBSOCK_ERROR_SOCKET,
	WEBSOCK_ERROR_STRLEN,
	WEBSOCK_ERROR_CONTINUE,
	WEBSOCK_ERROR_HTTPVER,
	WEBSOCK_ERROR_INVKEY,
	WEBSOCK_ERROR_PROTOVER,
	WEBSOCK_ERROR_NOTWS,
	WEBSOCK_ERROR_HASHINIT,
	WEBSOCK_ERROR_HEADER,
	WEBSOCK_ERROR_MASK,
	WEBSOCK_ERROR_PAYLOAD_TOO_BIG,
} EWebSockErrors;

typedef struct _WebShake {
	EWebShakeState state;
	cs_char line[1024];
	cs_char key[32];
	cs_int32 keylen;
	cs_int16 hdrflags;
} WebShake;

typedef struct _WebSock {
	WebShake shake;
	EWebSockState state;
	EWebSockErrors error;
	cs_uint16 paylen;
	cs_uint16 maxpaylen;
	cs_str proto;
	cs_char *payload;
	cs_char mask[4];
	cs_byte opcode;
	cs_bool done;
} WebSock;
#endif
