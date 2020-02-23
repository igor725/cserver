#include "core.h"
#include "platform.h"
#include "str.h"
#include "list.h"

AListField* AList_AddField(AListField** head, void* value) {
	AListField* currHead = *head;
	AListField* ptr = Memory_Alloc(1, sizeof(AListField));
	ptr->value.ptr = value;
	if(currHead) currHead->next = ptr;
	ptr->prev = currHead;
	*head = ptr;
	return ptr;
}

void AList_Remove(AListField** head, AListField* field) {
	if(field->next)
		field->next->prev = field->prev;
	else
		*head = field->prev;
	if(field->prev)
		field->prev->next = field->next;
	Memory_Free(field);
}

KListField* KList_Add(KListField** head, void* key, void* value) {
	KListField* currHead = *head;
	KListField* ptr = Memory_Alloc(1, sizeof(KListField));
	ptr->key.ptr = key;
	ptr->value.ptr = value;
	if(currHead) currHead->next = ptr;
	ptr->prev = currHead;
	*head = ptr;
	return ptr;
}

void KList_Remove(KListField** head, KListField* field) {
	if(field->next)
		field->next->prev = field->prev;
	else
		*head = field->prev;
	if(field->prev)
		field->prev->next = field->next;
	Memory_Free(field);
}
