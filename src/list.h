#ifndef LIST_H
#define LIST_H
#include "core.h"

union _MultiValue {
	cs_byte num8;
	cs_uint16 num16;
	cs_uint32 num32;
	cs_uintptr numptr;
	cs_str str;
	void *ptr;
};

typedef struct _AListField {
	union _MultiValue value;
	struct _AListField *next, *prev;
} AListField;

typedef struct _KListField {
	union _MultiValue key, value;
	struct _KListField *next, *prev;
} KListField;

#define List_Iter(field, head) \
for(field = head; field || (field && field->prev); field = field->prev)

API AListField *AList_AddField(AListField **head, void *value);
API void AList_Remove(AListField **head, AListField *field);

API KListField *KList_Add(KListField **head, void *key, void *value);
API void KList_Remove(KListField **head, KListField *field);
#endif
