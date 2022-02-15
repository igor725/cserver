#ifndef COMPRTYPES_H
#define COMPRTYPES_H
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
#endif
