#include "core.h"
#include "compr.h"
#include "platform.h"
#include <zlib.h>

INL static cs_int32 getWndBits(ComprType type) {
	switch(type) {
		case COMPR_TYPE_DEFLATE:
		case COMPR_TYPE_INFLATE:
			return -15;
		case COMPR_TYPE_UNGZIP:
		case COMPR_TYPE_GZIP:
			return 31;
		case COMPR_TYPE_NOTSET:
		default:
			return 0;
	}
}

cs_bool Compr_Init(Compr *ctx, ComprType type) {
	if(!ctx->stream) ctx->stream = Memory_Alloc(1, sizeof(z_stream));
	ctx->state = COMPR_STATE_IDLE;
	ctx->type = type;

	if(type == COMPR_TYPE_DEFLATE || type == COMPR_TYPE_GZIP) {
		ctx->ret = deflateInit2(
			ctx->stream, Z_DEFAULT_COMPRESSION,
			Z_DEFLATED, getWndBits(type),
			MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY
		);
	} else if(type == COMPR_TYPE_INFLATE || type == COMPR_TYPE_UNGZIP) {
		ctx->ret = inflateInit2(ctx->stream, getWndBits(type));
	}
	
	return ctx->ret == Z_OK;
}

void Compr_SetInBuffer(Compr *ctx, void *data, cs_uint32 size) {
	z_stream *stream = (z_stream *)ctx->stream;
	stream->avail_in = size;
	stream->next_in = data;
}

void Compr_SetOutBuffer(Compr *ctx, void *data, cs_uint32 size) {
	z_stream *stream = (z_stream *)ctx->stream;
	stream->avail_out = size;
	stream->next_out = data;
}

INL static cs_bool DeflateStep(Compr *ctx) {
	z_stream *stream = (z_stream *)ctx->stream;
	cs_uint32 outbuf_size = stream->avail_out;

	ctx->ret = deflate(stream, stream->avail_in > 0 ? Z_NO_FLUSH : Z_FINISH);

	if(stream->avail_out == outbuf_size) {
		ctx->state = COMPR_STATE_DONE;
		ctx->queued = ctx->written = 0;
	} else {
		ctx->written = outbuf_size - stream->avail_out;
		ctx->queued = stream->avail_in;
	}

	return true;
}

INL static cs_bool InflateStep(Compr *ctx) {
	z_stream *stream = (z_stream *)ctx->stream;
	ctx->written = 0;

	cs_uint32 avail = stream->avail_out;
	ctx->ret = inflate(stream, Z_NO_FLUSH);
	if(ctx->ret == Z_NEED_DICT || ctx->ret == Z_DATA_ERROR ||
	ctx->ret == Z_MEM_ERROR) return false;
	ctx->written = avail - stream->avail_out;
	ctx->queued = stream->avail_in;
	return true;
}

cs_bool Compr_Update(Compr *ctx) {
	if(ctx->state == COMPR_STATE_IDLE)
		ctx->state = COMPR_STATE_INPROCESS;
	else if(ctx->state == COMPR_STATE_DONE)
		return true;

	if(ctx->type == COMPR_TYPE_DEFLATE || ctx->type == COMPR_TYPE_GZIP)
		return DeflateStep(ctx);
	else if(ctx->type == COMPR_TYPE_INFLATE || ctx->type == COMPR_TYPE_UNGZIP)
		return InflateStep(ctx);
	
	return false;
}

void Compr_Reset(Compr *ctx) {
	if(ctx->stream) {
		if(ctx->type == COMPR_TYPE_DEFLATE || ctx->type == COMPR_TYPE_GZIP)
			deflateEnd(ctx->stream);
		else if(ctx->type == COMPR_TYPE_INFLATE || ctx->type == COMPR_TYPE_UNGZIP)
			inflateEnd(ctx->stream);
		Memory_Zero(ctx->stream, sizeof(z_stream));
	}
	ctx->type = COMPR_TYPE_NOTSET;
	ctx->state = COMPR_STATE_IDLE;
}

void Compr_Cleanup(Compr *ctx) {
	if(ctx->stream) {
		Memory_Free(ctx->stream);
		ctx->stream = NULL;
	}
}
