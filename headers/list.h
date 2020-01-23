#ifndef LIST_H
#define LIST_H
typedef struct AListField {
	void* value;
	struct AListField* next;
	struct AListField* prev;
} AListField;

typedef struct KListField {
	union {
		cs_str str;
		cs_uintptr num;
		void* ptr;
	} key;
	void* value;
	struct KListField* next;
	struct KListField* prev;
} KListField;

#define List_Iter(field, head) \
for(field = *head; field || (field && field->prev); field = field->prev)

AListField* AList_AddField(AListField** head, void* value);
void AList_Remove(AListField** head, AListField* field);

KListField* KList_Add(KListField** head, void* key, void* value);
void KList_Remove(KListField** head, KListField* field);
#endif
