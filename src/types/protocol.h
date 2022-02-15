#ifndef PROTOCOLTYPES_H
#define PROTOCOLTYPES_H
#include "core.h"
#include "types/client.h"

typedef cs_bool(*packetHandler)(Client *, cs_char *);

typedef enum _EEntProp {
	ROT_X = 0, // Вращение модели по оси X
	ROT_Y,
	ROT_Z
} EEntProp;

typedef struct _Packet {
	cs_byte id;
	cs_bool haveCPEImp;
	cs_uint16 size, extSize;
	cs_ulong exthash;
	cs_int32 extVersion;
	packetHandler handler;
	packetHandler extHandler;
} Packet;
#endif
