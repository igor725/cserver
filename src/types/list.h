#ifndef LISTTYPES_H
#define LISTTYPES_H
#include "core.h"

typedef union _MultiValue {
	cs_byte num8;
	cs_uint16 num16;
	cs_uint32 num32;
	cs_uintptr numptr;
	cs_str str;
	void *ptr;
} UMultiValue;

typedef struct _AListField {
	UMultiValue value;
	struct _AListField *next, *prev;
} AListField;

typedef struct _KListField {
	UMultiValue key, value;
	struct _KListField *next, *prev;
} KListField;
#endif
