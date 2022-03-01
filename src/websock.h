#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include "core.h"
#include "types/websock.h"

API cs_bool WebSock_DoHandshake(WebSock *ws);
API cs_bool WebSock_ReceiveFrame(WebSock *ws);
API cs_bool WebSock_SendFrame(WebSock *ws, cs_byte opcode, const cs_char *buf, cs_uint16 len);
API cs_str WebSock_GetError(WebSock *ws);
#endif // WEBSOCKET_H
