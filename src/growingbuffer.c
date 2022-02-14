#include "core.h"
#include "platform.h"
#include "growingbuffer.h"

void GrowingBuffer_SetMaxSize(GrowingBuffer *self, cs_uint32 maxsize) {
	self->maxsize = maxsize;
}

cs_bool GrowingBuffer_Ensure(GrowingBuffer *self, cs_uint32 size) {
	cs_uint32 required = self->offset + size;
	if(self->maxsize > 0 && self->maxsize < required)
		return false;

	if(self->size < required) {
		if(self->maxsize)
			required = min(required + GROWINGBUFFER_ADDITIONAL, self->maxsize);
		else
			required += GROWINGBUFFER_ADDITIONAL;

		if(self->buffer)
			self->buffer = Memory_Realloc(self->buffer, required);
		else
			self->buffer = Memory_Alloc(1, required);

		self->size = required;
	}

	return true;
}

cs_char *GrowingBuffer_GetCurrentPoint(GrowingBuffer *self) {
	return self->buffer + self->offset;
}

cs_uint32 GrowingBuffer_GetDiff(GrowingBuffer *self, cs_char *point) {
	if(point < self->buffer || point > (self->buffer + self->size))
		return 0;
	return (cs_uint32)(point - (self->buffer + self->offset));
}

cs_bool GrowingBuffer_Commit(GrowingBuffer *self, cs_uint32 size) {
	if(self->offset + size > self->size) return false;
	self->offset += size;
	return true;
}

cs_char *GrowingBuffer_PopFullData(GrowingBuffer *self, cs_uint32 *size) {
	if(size) *size = self->offset;
	self->offset = 0;
	return self->buffer;
}

void GrowingBuffer_Cleanup(GrowingBuffer *self) {
	if(self->buffer) {
		Memory_Free(self->buffer);
		self->buffer = NULL;
		self->size = 0;
	}
}
