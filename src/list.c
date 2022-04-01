#include "core.h"
#include "platform.h"
#include "list.h"

AListField *AList_AddField(AListField **head, void *value) {
	AListField *ptr = Memory_Alloc(1, sizeof(AListField));
	ptr->value.ptr = value;
	if(*head) (*head)->next = ptr;
	ptr->prev = *head;
	*head = ptr;
	return ptr;
}

cs_bool AList_Iter(AListField **head, void *ud, cs_bool(*callback)(AListField *, AListField **, void *)) {
	AListField *tmp;

	List_Iter(tmp, (*head))
		if(!callback(tmp, head, ud)) return false;

	return true;
}

UMultiValue AList_GetValue(AListField *field) {
	return field->value;
}

void AList_Remove(AListField **head, AListField *field) {
	if(field->next)
		field->next->prev = field->prev;
	else
		*head = field->prev;
	if(field->prev)
		field->prev->next = field->next;
	Memory_Free(field);
}

KListField *KList_AddField(KListField **head, void *key, void *value) {
	KListField *ptr = Memory_Alloc(1, sizeof(KListField));
	ptr->key.ptr = key;
	ptr->value.ptr = value;
	if(*head) (*head)->next = ptr;
	ptr->prev = *head;
	*head = ptr;
	return ptr;
}

cs_bool KList_Iter(KListField **head, void *ud, cs_bool(*callback)(KListField *, KListField **, void *)) {
	KListField *tmp = NULL;

	List_Iter(tmp, (*head))
		if(!callback(tmp, head, ud)) return false;

	return true;
}

UMultiValue KList_GetKey(KListField *field) {
	return field->key;
}

UMultiValue KList_GetValue(KListField *field) {
	return field->value;
}

void KList_Remove(KListField **head, KListField *field) {
	if(field->next)
		field->next->prev = field->prev;
	else
		*head = field->prev;
	if(field->prev)
		field->prev->next = field->next;
	Memory_Free(field);
}
