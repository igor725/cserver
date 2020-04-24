#ifndef LIST_H
#define LIST_H
typedef struct _AListField {
	union {
		cs_byte num8;
		cs_uint16 num16;
		cs_uint32 num32;
		cs_uintptr numptr;
		cs_str str;
		void *ptr;
	} value;
	struct _AListField *next, *prev;
} AListField;

typedef struct _KListField {
	union {
		cs_byte num8;
		cs_uint16 num16;
		cs_uint32 num32;
		cs_uintptr numptr;
		cs_str str;
		void *ptr;
	} key;
	union {
		cs_byte num8;
		cs_uint16 num16;
		cs_uint32 num32;
		cs_uintptr numptr;
		cs_str str;
		void *ptr;
	} value;
	struct _KListField *next, *prev;
} KListField;

#define List_Iter(field, head) \
for(field = head; field || (field && field->prev); field = field->prev)

AListField *AList_AddField(AListField **head, void *value);
void AList_Remove(AListField **head, AListField *field);

KListField *KList_Add(KListField **head, void *key, void *value);
void KList_Remove(KListField **head, KListField *field);
#endif
