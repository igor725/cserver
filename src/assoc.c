#include "core.h"
#include "client.h"
#include "world.h"
#include "assoc.h"
#include "platform.h"
#include "list.h"

AListField *headAssocType = NULL;

typedef struct AssocBind {
	EAssocBindType type;
	AssocType id;
} AssocBind;

INL static AssocBind *GetBind(AListField *a) {
	return a ? (AssocBind *)a->value.ptr : NULL;
}

INL static AListField *AGetType(AssocType type, AssocBind **bind) {
	AListField *ptr = NULL;

	List_Iter(ptr, headAssocType) {
		*bind = GetBind(ptr);
		if((*bind)->id == type) return ptr;
	}

	*bind = NULL;
	return NULL;
}

INL static KListField *AGetNode(void *target, AssocBind *bind) {
	KListField *ptr;

	switch(bind->type) {
		case ASSOC_BIND_CLIENT:
			List_Iter(ptr, ((Client *)target)->headNode)
				if(ptr->key.num16 == bind->id) return ptr;
			break;
		case ASSOC_BIND_WORLD:
			List_Iter(ptr, ((World *)target)->headNode)
				if(ptr->key.num16 == bind->id) return ptr;
			break;

		default: return NULL;
	}

	return NULL;
}

AssocType Assoc_NewType(EAssocBindType bindto) {
	AssocBind *headBind = GetBind(headAssocType);
	AssocBind *next = (AssocBind *)Memory_Alloc(1, sizeof(AssocBind));
	next->id = headBind ? headBind->id + 1 : 0;
	if(next->id < 0) {
		Memory_Free(next);
		return (AssocType)-1;
	}
	next->type = bindto;
	AList_AddField(&headAssocType, next);
	return next->id;
}

cs_bool Assoc_DelType(AssocType type) {
	AssocBind *bind = NULL;
	AListField *tptr = AGetType(type, &bind), *witer;
	if(!tptr) return false;

	switch(bind->type) {
		case ASSOC_BIND_CLIENT:
			for(ClientID id = 0; id < MAX_CLIENTS; id++) {
				if(Clients_List[id])
					Assoc_Remove(Clients_List[id], type);
			}
			break;
		case ASSOC_BIND_WORLD:
			List_Iter(witer, World_Head)
				Assoc_Remove(witer->value.ptr, type);
			break;

		default: return false;
	}

	Memory_Free(bind);
	AList_Remove(&headAssocType, tptr);
	return true;
}

void *Assoc_AllocFor(void *target, AssocType type, cs_size num, cs_size size) {
	AssocBind *bind = NULL;
	if(!AGetType(type, &bind) || AGetNode(target, bind)) return NULL;
	void *memptr = Memory_TryAlloc(num, size);
	if(!memptr) return NULL;
	KListField *anode = NULL;

	switch(bind->type) {
		case ASSOC_BIND_CLIENT:
			anode = KList_AddField(&((Client *)target)->headNode, NULL, NULL);
			break;
		case ASSOC_BIND_WORLD:
			anode = KList_AddField(&((World *)target)->headNode, NULL, NULL);
			break;

		default: return NULL;
	}

	anode->value.ptr = memptr;
	anode->key.num16 = type;
	return memptr;
}

void *Assoc_GetPtr(void *target, AssocType type) {
	AssocBind *bind = NULL;
	if(!AGetType(type, &bind)) return NULL;
	KListField *anode = AGetNode(target, bind);
	if(anode) return anode->value.ptr;
	return NULL;
}

cs_bool Assoc_Remove(void *target, AssocType type) {
	AssocBind *bind = NULL;
	if(!AGetType(type, &bind)) return false;
	KListField *anode = AGetNode(target, bind);
	if(!anode) return false;
	Memory_Free(anode->value.ptr);

	switch(bind->type) {
		case ASSOC_BIND_CLIENT:
			KList_Remove(&((Client *)target)->headNode, anode);
			break;
		case ASSOC_BIND_WORLD:
			KList_Remove(&((World *)target)->headNode, anode);
			break;
	}

	return true;
}
