#ifndef COMPR_H
#define COMPR_H
#include "core.h"

typedef enum _ComprType {
	COMPR_TYPE_NOTSET,
	COMPR_TYPE_DEFLATE,
	COMPR_TYPE_INFLATE,
	COMPR_TYPE_UNGZIP,
	COMPR_TYPE_GZIP
} ComprType;

typedef enum _ComprState {
	COMPR_STATE_IDLE,
	COMPR_STATE_INPROCESS,
	COMPR_STATE_FINISHING,
	COMPR_STATE_DONE
} ComprState;

typedef struct _Compr {
	ComprState state;
	ComprType type;
	cs_int32 ret;
	cs_uint32 written, queued;
	void *stream;
} Compr;

API cs_bool Compr_Init(Compr *ctx, ComprType type);
API cs_ulong Compr_CRC32(const cs_byte *data, cs_uint32 len);
API void Compr_SetInBuffer(Compr *ctx, void *data, cs_uint32 size);
API void Compr_SetOutBuffer(Compr *ctx, void *data, cs_uint32 size);
API cs_bool Compr_Update(Compr *ctx);
API cs_str Compr_GetError(cs_int32 code);
API void Compr_Reset(Compr *ctx);
API void Compr_Cleanup(Compr *ctx);
void Compr_Uninit(void);
#endif
