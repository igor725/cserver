#include "core.h"
#include "platform.h"
#include "netbuffer.h"
#include "websock.h"

static cs_bool Ensure(GrowingBuffer *self, cs_uint32 size) {
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

static void Cleanup(GrowingBuffer *self) {
	if(self->buffer) {
		Memory_Free(self->buffer);
		self->buffer = NULL;
		self->offset = 0;
		self->size = 0;
	}
}

void NetBuffer_Init(NetBuffer *nb, Socket sock) {
	Ensure(&nb->write, 1);
	Ensure(&nb->read, 1);
	nb->fd = sock;
}

cs_bool NetBuffer_Process(NetBuffer *nb) {
	cs_ulong avail = Socket_AvailData(nb->fd);
	if(avail > 0) {
		Ensure(&nb->read, avail);
		cs_char *data = nb->read.buffer + nb->read.offset;
		if(Socket_Receive(nb->fd, data, avail, 0) != -1)
			nb->read.offset += avail;
		else if(Socket_IsFatal()) {
			nb->closed = true;
			return false;
		}
	} else {
		cs_char unused;
		cs_int32 ret = Socket_Receive(nb->fd, &unused, 1, MSG_PEEK);
		if(ret == 0 || (ret == -1 && Socket_IsFatal())) {
			nb->closed = true;
			return false;
		}
	}

	avail = NetBuffer_AvailWrite(nb);
	if(avail > 0) {
		if(nb->asframe) {
			if(nb->framesize == 0) {
				cs_int32 hdrlen = 0;
				nb->framesize = avail;
				if(WebSock_WriteHeader(nb->fd, 0x02, avail, &hdrlen) != hdrlen) {
					if(Socket_IsFatal()) {
						nb->closed = true;
						return false;
					}
				}
			}
		}
		cs_char *data = nb->write.buffer + nb->cwrite;
		cs_int32 sent = Socket_Send(nb->fd, data, avail);
		if(sent > 0) {
			nb->cwrite += sent;
			if(nb->asframe) nb->framesize -= sent;
			if(NetBuffer_AvailWrite(nb) == 0) {
				if(nb->shutdown) Socket_Shutdown(nb->fd, SD_SEND);
				nb->write.offset = 0;
				nb->cwrite = 0;
			}
		} else if(Socket_IsFatal()) {
			nb->closed = true;
			return false;
		}
	}

	return true;
}

cs_char *NetBuffer_PeekRead(NetBuffer *nb, cs_uint32 point) {
	if(nb->cread + point > nb->read.offset) return NULL;
	return nb->read.buffer + nb->cread;
}

// TODO: Почистить эту функцию от непотребностей
cs_int32 NetBuffer_ReadLine(NetBuffer *nb, cs_char *buffer, cs_uint32 buflen) {
	cs_char *data = nb->read.buffer + nb->cread;
	if(!data) {
		nb->read.offset = 0;
		return -2;
	}

	cs_uint32 avail = nb->read.offset - nb->cread;
	if(avail == 0) return -2;

	cs_uint32 bufpos = 0;
	for(cs_uint32 i = 0; i < buflen; i++) {
		if(i > avail) return -2;
 
		if(data[i] == '\n') {
			nb->cread += (i + 1);
			buffer[bufpos++] = '\0';
			return (cs_int32)i;
		} else if(data[i] != '\r')
			buffer[bufpos++] = data[i];
	}

	return -1;
}

cs_char *NetBuffer_Read(NetBuffer *nb, cs_uint32 len) {
	cs_char *ptr = nb->read.buffer + nb->cread;
	if(!ptr) {
		nb->read.offset = 0;
		return false;
	}
	nb->cread += len;
	return ptr;
}

cs_char *NetBuffer_StartWrite(NetBuffer *nb, cs_uint32 dlen) {
	if(!Ensure(&nb->write, dlen)) return false;
	return nb->write.buffer + nb->write.offset;
}

cs_bool NetBuffer_EndWrite(NetBuffer *nb, cs_uint32 size) {
	cs_uint32 newsize = nb->write.offset + size;
	if(newsize > nb->write.size) return false;
	nb->write.offset = newsize;
	return true;
}

cs_uint32 NetBuffer_AvailRead(NetBuffer *nb) {
	return nb->read.offset - nb->cread;
}

cs_uint32 NetBuffer_AvailWrite(NetBuffer *nb) {
	return nb->write.offset - nb->cwrite;
}

cs_bool NetBuffer_Shutdown(NetBuffer *nb) {
	if(!NetBuffer_IsAlive(nb) || !NetBuffer_IsValid(nb)) return false;
	nb->shutdown = true;
	return true;
}

cs_bool NetBuffer_IsValid(NetBuffer *nb) {
	return nb->fd != INVALID_SOCKET;
}

cs_bool NetBuffer_IsAlive(NetBuffer *nb) {
	return nb->closed == false;
}

void NetBuffer_ForceClose(NetBuffer *nb) {
	if(nb->closed) return;
	if(nb->fd != INVALID_SOCKET) Socket_Close(nb->fd);
	Cleanup(&nb->write);
	Cleanup(&nb->read);
	nb->closed = true;
}
