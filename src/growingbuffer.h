#ifndef GROWINGBUFFER_H
#define GROWINGBUFFER_H
#include "core.h"

#define GROWINGBUFFER_ADDITIONAL 512

typedef struct _GrowingBuffer {
	cs_uint32 offset, size;
	cs_uint32 maxsize;
	cs_char *buffer;
} GrowingBuffer;

API void GrowingBuffer_SetMaxSize(GrowingBuffer *self, cs_uint32 maxsize);
API cs_bool GrowingBuffer_Ensure(GrowingBuffer *self, cs_uint32 size);
API cs_char *GrowingBuffer_GetCurrentPoint(GrowingBuffer *self);
API cs_uint32 GrowingBuffer_GetDiff(GrowingBuffer *self, cs_char *point);
API cs_bool GrowingBuffer_Commit(GrowingBuffer *self, cs_uint32 size);
API cs_char *GrowingBuffer_PopFullData(GrowingBuffer *self, cs_uint32 *size);
API void GrowingBuffer_Cleanup(GrowingBuffer *self);
#endif
