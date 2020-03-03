#ifndef LIST_H
#define LIST_H
typedef struct AListField {
	union {
		cs_uint8 num8;
		cs_uint16 num16;
		cs_uint32 num32;
		cs_uintptr numptr;
		cs_str str;
		void* ptr;
	} value;
	struct AListField* next;
	struct AListField* prev;
} AListField;

typedef struct KListField {
	union {
		cs_uint8 num8;
		cs_uint16 num16;
		cs_uint32 num32;
		cs_uintptr numptr;
		cs_str str;
		void* ptr;
	} key;
	union {
		cs_uint8 num8;
		cs_uint16 num16;
		cs_uint32 num32;
		cs_uintptr numptr;
		cs_str str;
		void* ptr;
	} value;
	struct KListField* next;
	struct KListField* prev;
} KListField;

#define List_Iter(field, head) \
for(field = head; field || (field && field->prev); field = field->prev)

AListField* AList_AddField(AListField** head, void* value);
void AList_Remove(AListField** head, AListField* field);

KListField* KList_Add(KListField** head, void* key, void* value);
void KList_Remove(KListField** head, KListField* field);
#endif
