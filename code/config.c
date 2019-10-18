#include "core.h"
#include "config.h"

const char* commentSymbol = "#";

CFGSTORE Config_Create(const char* filename) {
	CFGSTORE store = Memory_Alloc(1, sizeof(struct cfgStore));
	store->path = String_AllocCopy(filename);
	return store;
}

CFGENTRY Config_GetEntry(CFGSTORE store, const char* key) {
	CFGENTRY ent = store->firstCfgEntry;

	while(ent) {
		if(String_CaselessCompare(ent->key, key)) {
			return ent;
		}
		ent = ent->next;
	}

	return ent;
}

CFGENTRY Config_NewEntry(CFGSTORE store, const char* key) {
	CFGENTRY ent = Memory_Alloc(1, sizeof(struct cfgEntry));
	ent->key = String_AllocCopy(key);

	if(store->firstCfgEntry)
		store->lastCfgEntry->next = ent;
	else
		store->firstCfgEntry = ent;

	store->lastCfgEntry = ent;
	return ent;
}

static void EmptyEntry(CFGENTRY ent) {
	if(ent->type == CFG_STR && ent->value.vchar)
		Memory_Free((void*)ent->value.vchar);
	if(ent->type == CFG_STR && ent->defvalue.vchar)
		Memory_Free((void*)ent->defvalue.vchar);

	ent->type = -1;
	ent->changed = false;
	ent->value.vchar = NULL;
	ent->defvalue.vchar = NULL;
}

static bool AllCfgEntriesParsed(CFGSTORE store) {
	CFGENTRY ent = store->firstCfgEntry;
	bool loaded = true;

	while(ent && loaded) {
		loaded = ent->changed;
		ent = ent->next;
	}

	return loaded;
}

const char* Config_TypeName(int type) {
	switch (type) {
		case CFG_STR:
			return "string";
		case CFG_INT:
			return "integer";
		case CFG_BOOL:
			return "boolean";
		default:
			return "unknownType";
	}
}

#define CFG_TYPE(expectedType) \
if(ent->type != expectedType) { \
	Error_PrintF2(ET_SERVER, EC_CFGINVGET, true, ent->key, store->path, Config_TypeName(expectedType), Config_TypeName(ent->type)); \
}

#define CFG_CHECKENTRY(store, ent) \
if(!ent) { \
	Error_PrintF2(ET_SERVER, EC_CFGUNK, true, key, store->path); \
} \

int Config_TypeNameToInt(const char* name) {
	if(String_CaselessCompare(name, "string")) {
		return CFG_STR;
	} else if(String_CaselessCompare(name, "integer")) {
		return CFG_INT;
	} else if(String_CaselessCompare(name, "boolean")) {
		return CFG_BOOL;
	}
	return -1;
}

bool Config_Load(CFGSTORE store) {
	FILE* fp = File_Open(store->path, "r");
	if(!fp) {
		if(errno == ENOENT) return true;
		Error_PrintSys(false);
		return false;
	}

	int type;
	int count = 0;
	int ch = fgetc(fp);
	bool haveCommentary = false;
	char key[MAX_CFG_LEN] = {0};
	char value[MAX_CFG_LEN] = {0};
	char commentary[MAX_CFG_LEN] = {0};

	while(!feof(fp)) {
		while(ch == '\n' && !feof(fp))
			ch = fgetc(fp);

		if(ch == *commentSymbol) {
			while(ch != '\n' && !feof(fp) && count < MAX_CFG_LEN) {
				ch = fgetc(fp);
				if(ch != '\n' && ch != '\r')
					commentary[count++] = (char)ch;
			}
			commentary[count] = '\0';
			haveCommentary = true;
			count = 0;
		}

		do {
			if(ch != '\n' && ch != '\r' && ch != ' ')
				key[count++] = (char)ch;
			ch = fgetc(fp);
		} while(ch != '=' && !feof(fp) && count < MAX_CFG_LEN);
		key[count] = '\0';

		if(feof(fp)) {
			Error_PrintF2(ET_SERVER, EC_CFGEND, false, store->path);
			return false;
		}

		count = 0;
		type = fgetc(fp);

		while((ch = fgetc(fp)) != EOF && ch != '\n' && count < MAX_CFG_LEN) {
			if(ch != '\r') value[count++] = (char)ch;
		}
		value[count] = '\0';

		if(count < 1) {
			Error_PrintF2(ET_SERVER, EC_CFGEND, false, store->path);
			File_Close(fp);
			return false;
		}

		CFGENTRY ent = Config_GetEntry(store, key);
		if(!ent) {
			Error_PrintF2(ET_SERVER, EC_CFGUNK, false, key, store->path);
			File_Close(fp);
			return false;
		}

		switch (type) {
			case CFG_STR:
				Config_SetStr(ent, value);
				break;
			case CFG_INT:
				Config_SetInt(ent, String_ToInt(value));
				break;
			case CFG_BOOL:
				Config_SetBool(ent, String_Compare(value, "True"));
				break;
			default:
				Error_PrintF2(ET_SERVER, EC_CFGTYPE, false, store->path, type);
				File_Close(fp);
				return false;
		}

		if(haveCommentary)
			Config_SetComment(ent, commentary);

		count = 0;
		ch = fgetc(fp);
		haveCommentary = false;
	}

	store->modified = !AllCfgEntriesParsed(store);
	File_Close(fp);
	return true;
}

bool Config_Save(CFGSTORE store) {
	if(!store->modified) return true;

	char tmpname[256];
	String_FormatBuf(tmpname, 256, "%s.tmp", store->path);

	FILE* fp = File_Open(tmpname, "w");
	if(!fp) {
		Error_PrintSys(false);
		return false;
	}

	CFGENTRY ptr = store->firstCfgEntry;
	store->modified = false;

	while(ptr) {
		if(ptr->commentary)
			if(!File_WriteFormat(fp, "#%s\n", ptr->commentary))
				return false;
		if(!File_Write(ptr->key, String_Length(ptr->key), 1, fp))
			return false;

		char* vchar;
		int vint;
		bool vbool;

		switch (ptr->type) {
			case CFG_STR:
				vchar = (char*)(ptr->changed ? ptr->value.vchar : ptr->defvalue.vchar);
				if(!File_WriteFormat(fp, "=s%s\n", vchar))
					return false;
				break;
			case CFG_INT:
				vint = ptr->changed ? ptr->value.vint : ptr->defvalue.vint;
				if(!File_WriteFormat(fp, "=i%d\n", vint))
					return false;
				break;
			case CFG_BOOL:
				vbool = ptr->changed ? ptr->value.vbool : ptr->defvalue.vbool;
				if(!File_WriteFormat(fp, "=b%s\n", vbool ? "True" : "False"))
					return false;
				break;
			default:
				if(!File_Write("=sUnknown value\n", 16, 1, fp))
					return false;
				break;
		}
		ptr = ptr->next;
	}

	File_Close(fp);
	return File_Rename(tmpname, store->path);
}

void Config_SetComment(CFGENTRY ent, const char* commentary) {
	if(ent->commentary)
		Memory_Free((void*)ent->commentary);
	ent->commentary = String_AllocCopy(commentary);
}

void Config_SetDefaultInt(CFGENTRY ent, int value) {
	EmptyEntry(ent);
	ent->type = CFG_INT;
	ent->defvalue.vint = value;
}

void Config_SetInt(CFGENTRY ent, int value) {
	EmptyEntry(ent);
	ent->type = CFG_INT;
	ent->changed = true;
	ent->value.vint = value;
}

void Config_SetIntByKey(CFGSTORE store, const char* key, int value) {
	CFGENTRY ent = Config_GetEntry(store, key);
	CFG_CHECKENTRY(store, ent);
	Config_SetInt(ent, value);
	store->modified = true;
}

int Config_GetInt(CFGSTORE store, const char* key) {
	CFGENTRY ent = Config_GetEntry(store, key);
	CFG_CHECKENTRY(store, ent);
	CFG_TYPE(CFG_INT);
	return ent->changed ? ent->value.vint : ent->defvalue.vint;
}

void Config_SetDefaultStr(CFGENTRY ent, const char* value) {
	EmptyEntry(ent);
	ent->type = CFG_STR;
	ent->defvalue.vchar = String_AllocCopy(value);
}

void Config_SetStr(CFGENTRY ent, const char* value) {
	EmptyEntry(ent);
	ent->type = CFG_STR;
	ent->changed = true;
	ent->value.vchar = String_AllocCopy(value);
}

void Config_SetStrByKey(CFGSTORE store, const char* key, const char* value) {
	CFGENTRY ent = Config_GetEntry(store, key);
	CFG_CHECKENTRY(store, ent);
	Config_SetStr(ent, value);
	store->modified = true;
}

const char* Config_GetStr(CFGSTORE store, const char* key) {
	CFGENTRY ent = Config_GetEntry(store, key);
	CFG_CHECKENTRY(store, ent);
	CFG_TYPE(CFG_STR);
	return ent->changed ? ent->value.vchar : ent->defvalue.vchar;
}

void Config_SetDefaultBool(CFGENTRY ent, bool value) {
	EmptyEntry(ent);
	ent->type = CFG_BOOL;
	ent->defvalue.vbool = value;
}

void Config_SetBool(CFGENTRY ent, bool value) {
	EmptyEntry(ent);
	ent->type = CFG_BOOL;
	ent->changed = true;
	ent->value.vbool = value;
}

void Config_SetBoolByKey(CFGSTORE store, const char* key, bool value) {
	CFGENTRY ent = Config_GetEntry(store, key);
	CFG_CHECKENTRY(store, ent);
	Config_SetBool(ent, value);
	store->modified = true;
}

bool Config_GetBool(CFGSTORE store, const char* key) {
	CFGENTRY ent = Config_GetEntry(store, key);
	CFG_CHECKENTRY(store, ent);
	CFG_TYPE(CFG_BOOL);
	return ent->changed ? ent->value.vbool : ent->defvalue.vbool;
}

void Config_EmptyStore(CFGSTORE store) {
	CFGENTRY prev, ent = store->firstCfgEntry;

	while(ent) {
		prev = ent;
		ent = ent->next;
		if(prev->commentary)
			Memory_Free((void*)prev->commentary);
		EmptyEntry(prev);
		Memory_Free(prev);
	}

	store->modified = true;
	store->firstCfgEntry = NULL;
	store->lastCfgEntry = NULL;
}

void Config_DestroyStore(CFGSTORE store) {
	Memory_Free((void*)store->path);
	Config_EmptyStore(store);
	Memory_Free(store);
}
