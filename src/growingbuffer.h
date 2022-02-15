#ifndef GROWINGBUFFER_H
#define GROWINGBUFFER_H
#include "core.h"
#include "types/growingbuffer.h"

API void GrowingBuffer_SetMaxSize(GrowingBuffer *self, cs_uint32 maxsize);
API cs_bool GrowingBuffer_Ensure(GrowingBuffer *self, cs_uint32 size);
API cs_char *GrowingBuffer_GetCurrentPoint(GrowingBuffer *self);
API cs_uint32 GrowingBuffer_GetDiff(GrowingBuffer *self, cs_char *point);
API cs_bool GrowingBuffer_Commit(GrowingBuffer *self, cs_uint32 size);
API cs_char *GrowingBuffer_PopFullData(GrowingBuffer *self, cs_uint32 *size);
API void GrowingBuffer_Cleanup(GrowingBuffer *self);
#endif
