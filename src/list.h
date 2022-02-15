#ifndef LIST_H
#define LIST_H
#include "core.h"
#include "types/list.h"

#define List_Iter(field, head) \
for(field = head; field != NULL; field = field->prev)

API AListField *AList_AddField(AListField **head, void *value);
API cs_bool AList_Iter(AListField **head, void *ud, cs_bool(*callback)(AListField *, AListField **, void *));
API UMultiValue AList_GetValue(AListField *field);
API void AList_Remove(AListField **head, AListField *field);

API KListField *KList_AddField(KListField **head, void *key, void *value);
API cs_bool KList_Iter(KListField **head, void *ud, cs_bool(*callback)(KListField *, KListField **, void *));
API UMultiValue KList_GetKey(KListField *field);
API UMultiValue KList_GetValue(KListField *field);
API void KList_Remove(KListField **head, KListField *field);
#endif
