#include "core.h"
#include "compr.h"
#include "platform.h"
#include "strstor.h"
#include "log.h"
#include <zlib.h>

static struct _ZLib {
	void *lib;

	unsigned long(*crc32)(unsigned long start, const unsigned char *data, unsigned int len);
	unsigned long(*zflags)(void);
	char *(*error)(int code);

	int(*definit)(z_streamp strm, int level, int meth, int bits, int memlvl, int strat, const char *ver, int size);
	int(*deflate)(z_streamp strm, int flush);
	int(*defend)(z_streamp strm);

	int(*infinit)(z_streamp strm, int bits, const char *ver, int size);
	int(*inflate)(z_streamp strm, int flush);
	int(*infend)(z_streamp strm);
} zlib;

static cs_str zsmylist[] = {
	"crc32", "zlibCompileFlags", "zError",
	"deflateInit2_", "deflate", "deflateEnd",
	"inflateInit2_", "inflate", "inflateEnd",
	NULL
};

static cs_str zlibdll[] = {
#if defined(CORE_USE_WINDOWS)
	"zlibwapi.dll",
	"zlib1.dll",
	"zlib.dll",
	"libz.dll",
#elif defined(CORE_USE_UNIX)
	"libz.so",
	"libz.so.1",
#else
#	error This file wants to be hacked
#endif
	NULL
};

INL static cs_bool InitBackend(void) {
	if(!zlib.lib && !DLib_LoadAll(zlibdll, zsmylist, (void **)&zlib))
		return false;

	cs_ulong flags = zlib.zflags();

	if(flags & BIT(17)) {
		Log_Error(Sstor_Get("Z_NOGZ"));
		return false;
	}

	if(flags & BIT(21)) {
		Log_Warn(Sstor_Get("Z_LVL1"));
		Log_Warn(Sstor_Get("Z_LVL2"));
		Log_Warn(Sstor_Get("Z_LVL3"));
	}

	return true;
}

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
	if(!zlib.lib && !InitBackend()) return false;

	if(!ctx->stream) ctx->stream = Memory_Alloc(1, sizeof(z_stream));
	ctx->state = COMPR_STATE_IDLE;
	ctx->type = type;

	if(type == COMPR_TYPE_DEFLATE || type == COMPR_TYPE_GZIP) {
		if(!zlib.definit) return false;
		ctx->ret = zlib.definit(
			ctx->stream, Z_DEFAULT_COMPRESSION,
			Z_DEFLATED, getWndBits(type),
			MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY,
			ZLIB_VERSION, sizeof(z_stream)
		);
	} else if(type == COMPR_TYPE_INFLATE || type == COMPR_TYPE_UNGZIP) {
		if(!zlib.infinit) return false;
		ctx->ret = zlib.infinit(ctx->stream, getWndBits(type), ZLIB_VERSION, sizeof(z_stream));
	}

	return ctx->ret == Z_OK;
}

cs_bool Compr_IsInState(Compr *ctx, ComprState state) {
	return ctx->state == state;
}

cs_ulong Compr_CRC32(const cs_byte *data, cs_uint32 len) {
	if(!zlib.crc32) return 0x00000000;
	return zlib.crc32(0, data, len);
}

void Compr_SetInBuffer(Compr *ctx, void *data, cs_uint32 size) {
	z_streamp stream = (z_streamp )ctx->stream;
	ctx->state = COMPR_STATE_INPROCESS;
	stream->avail_in = size;
	stream->next_in = data;
}

void Compr_SetOutBuffer(Compr *ctx, void *data, cs_uint32 size) {
	z_streamp stream = (z_streamp )ctx->stream;
	stream->avail_out = size;
	stream->next_out = data;
}

INL static cs_bool DeflateStep(Compr *ctx) {
	if(!zlib.deflate) return false;
	z_streamp stream = (z_streamp)ctx->stream;
	cs_uint32 outbuf_size = stream->avail_out;

	ctx->ret = zlib.deflate(stream, ctx->state == COMPR_STATE_FINISHING ? Z_FINISH : Z_NO_FLUSH);

	if(ctx->state == COMPR_STATE_FINISHING && stream->avail_out == outbuf_size)
		ctx->state = COMPR_STATE_DONE;
	else if(ctx->state == COMPR_STATE_INPROCESS && stream->avail_out == outbuf_size)
		ctx->state = COMPR_STATE_FINISHING;

	ctx->written = outbuf_size - stream->avail_out;
	ctx->queued = stream->avail_in;

	return true;
}

INL static cs_bool InflateStep(Compr *ctx) {
	z_streamp stream = (z_streamp)ctx->stream;
	cs_uint32 avail = stream->avail_out;
	ctx->written = 0;
	ctx->ret = zlib.inflate(stream, Z_NO_FLUSH);
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

cs_str Compr_GetLastError(Compr *ctx) {
	return Compr_GetError(ctx->ret);
}

cs_str Compr_GetError(cs_int32 code) {
	if(!zlib.error) return "zlib is not loaded correctly";
	return zlib.error(code);
}

cs_uint32 Compr_GetQueuedSize(Compr *ctx) {
	return ctx->queued;
}

cs_uint32 Compr_GetWrittenSize(Compr *ctx) {
	return ctx->written;
}

void Compr_Reset(Compr *ctx) {
	if(ctx->stream) {
		if(ctx->type == COMPR_TYPE_DEFLATE || ctx->type == COMPR_TYPE_GZIP)
			zlib.defend(ctx->stream);
		else if(ctx->type == COMPR_TYPE_INFLATE || ctx->type == COMPR_TYPE_UNGZIP)
			zlib.infend(ctx->stream);
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

void Compr_Uninit(void) {
	if(!zlib.lib) return;
	DLib_Unload(zlib.lib);
	Memory_Zero(&zlib, sizeof(zlib));
}
