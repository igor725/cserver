#ifndef PROTOCOLTYPES_H
#define PROTOCOLTYPES_H
#include "core.h"

typedef struct _Packet {
	cs_byte id;
	cs_bool haveCPEImp;
	cs_uint16 size, extSize;
	cs_ulong exthash;
	cs_int32 extVersion;
	void *handler;
	void *extHandler;
} Packet;
#endif
