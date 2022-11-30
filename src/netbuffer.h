#ifndef NETBUFFER_H
#define NETBUFFER_H
#include "core.h"
#include "types/netbuffer.h"

API void NetBuffer_Init(NetBuffer *nb, Socket sock);
API cs_bool NetBuffer_Process(NetBuffer *nb);
API cs_char *NetBuffer_PeekRead(NetBuffer *nb, cs_uint32 point);
API cs_int32 NetBuffer_ReadLine(NetBuffer *nb, cs_char *buffer, cs_uint32 buflen);
API cs_char *NetBuffer_Read(NetBuffer *nb, cs_uint32 len);
API cs_char *NetBuffer_StartWrite(NetBuffer *nb, cs_uint32 dlen);
API cs_bool NetBuffer_EndWrite(NetBuffer *nb, cs_uint32 dlen);
API cs_uint32 NetBuffer_AvailRead(NetBuffer *nb);
API cs_uint32 NetBuffer_AvailWrite(NetBuffer *nb);
API cs_bool NetBuffer_Shutdown(NetBuffer *nb);
API cs_bool NetBuffer_IsValid(NetBuffer *nb);
API cs_bool NetBuffer_IsAlive(NetBuffer *nb);
API void NetBuffer_ForceClose(NetBuffer *nb);
#endif
