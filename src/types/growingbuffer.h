#ifndef GROWINGBUFFERTYPES_H
#define GROWINGBUFFERTYPES_H
#include "core.h"

#define GROWINGBUFFER_ADDITIONAL 512

typedef struct _GrowingBuffer {
	cs_uint32 offset, size;
	cs_uint32 maxsize;
	cs_char *buffer;
} GrowingBuffer;
#endif
