#ifndef NETBUFFER_H
#define NETBUFFER_H
#include "core.h"
#include "types/netbuffer.h"

void NetBuffer_Init(NetBuffer *nb, Socket sock);
cs_bool NetBuffer_Process(NetBuffer *nb);
cs_char *NetBuffer_PeekRead(NetBuffer *nb, cs_uint32 point);
cs_int32 NetBuffer_ReadLine(NetBuffer *nb, cs_char *buffer, cs_uint32 buflen);
cs_char *NetBuffer_Read(NetBuffer *nb, cs_uint32 len);
cs_char *NetBuffer_StartWrite(NetBuffer *nb, cs_uint32 dlen);
cs_bool NetBuffer_EndWrite(NetBuffer *nb, cs_uint32 dlen);
cs_uint32 NetBuffer_AvailRead(NetBuffer *nb);
cs_uint32 NetBuffer_AvailWrite(NetBuffer *nb);
cs_bool NetBuffer_Shutdown(NetBuffer *nb);
cs_bool NetBuffer_IsValid(NetBuffer *nb);
cs_bool NetBuffer_IsAlive(NetBuffer *nb);
void NetBuffer_ForceClose(NetBuffer *nb);
#endif
