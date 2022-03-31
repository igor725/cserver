#ifndef NETBUFFERTYPES_H
#define NETBUFFERTYPES_H
#include "core.h"
#include "types/platform.h"

#define GROWINGBUFFER_ADDITIONAL 512

typedef struct _GrowingBuffer {
	cs_uint32 offset, size;
	cs_uint32 maxsize;
	cs_char *buffer;
} GrowingBuffer;

typedef struct _NetBuffer {
	Socket fd;
	cs_uint32 cread, cwrite;
	GrowingBuffer write;
	GrowingBuffer read;
	cs_bool closed;
	cs_bool shutdown;
	cs_bool asframe;
	cs_uint32 framesize;
} NetBuffer;
#endif
