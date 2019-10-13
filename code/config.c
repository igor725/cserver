#include "core.h"
#include "config.h"

CFGSTORE Config_Create(const char* filename) {
	CFGSTORE store = Memory_Alloc(1, sizeof(struct cfgStore));
	store->path = String_AllocCopy(filename);
	return store;
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
	char key[MAX_CLIENT_PPS] = {0};
	char value[MAX_CLIENT_PPS] = {0};

	while(!feof(fp)) {
		while(ch == '\n') {
			ch = fgetc(fp);
		}

		do {
			if(ch != '\n' && ch != '\r' && ch != ' ') {
				key[count] = (char)ch;
				count++;
			}
			ch = fgetc(fp);
		} while(ch != '=' && !feof(fp) && count < MAX_CLIENT_PPS);
		key[count] = '\0';

		if(feof(fp)) {
			Error_PrintF2(ET_SERVER, EC_CFGEND, false, store->path);
			return false;
		}

		count = 0;
		type = fgetc(fp);

		while((ch = fgetc(fp)) != EOF && ch != '\n' && count < MAX_CLIENT_PPS) {
			if(ch != '\r') {
				value[count] = (char)ch;
				count++;
			}
		}
		value[count] = '\0';

		if(count < 1) {
			Error_PrintF2(ET_SERVER, EC_CFGEND, false, store->path);
			File_Close(fp);
			return false;
		}

		switch (type) {
			case CFG_STR:
				Config_SetStr(store, key, value);
				break;
			case CFG_INT:
				Config_SetInt(store, key, String_ToInt(value));
				break;
			case CFG_BOOL:
				Config_SetBool(store, key, String_Compare(value, "True"));
				break;
			default:
				Error_PrintF2(ET_SERVER, EC_CFGTYPE, false, store->path, type);
				File_Close(fp);
				return false;
		}

		count = 0;
		ch = fgetc(fp);
	}

	store->modified = false;
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
		if(!File_Write(ptr->key, String_Length(ptr->key), 1, fp))
			return false;
		switch (ptr->type) {
			case CFG_STR:
				if(!File_WriteFormat(fp, "=s%s\n", (char*)ptr->value.vchar))
					return false;
				break;
			case CFG_INT:
				if(!File_WriteFormat(fp, "=i%d\n", (int)ptr->value.vint))
					return false;
				break;
			case CFG_BOOL:
				if(!File_WriteFormat(fp, "=b%s\n", ptr->value.vbool ? "True" : "False"))
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

CFGENTRY Config_GetEntry2(CFGSTORE store, const char* key) {
	CFGENTRY ent = Config_GetEntry(store, key);

	if(!ent) {
		ent = Memory_Alloc(1, sizeof(struct cfgEntry));
		ent->key = String_AllocCopy(key);

		if(store->firstCfgEntry)
			store->lastCfgEntry->next = ent;
		else
			store->firstCfgEntry = ent;

		store->lastCfgEntry = ent;
	}

	return ent;
}

static void cfgTypeChecker(CFGSTORE store, CFGENTRY ent, int expectedType) {
	if(ent->type != expectedType) {
		Error_PrintF2(ET_SERVER, EC_CFGINVGET, true, ent->key, store->path, expectedType, ent->type);
	}
}

void Config_SetInt(CFGSTORE store, const char* key, int value) {
	CFGENTRY ent = Config_GetEntry2(store, key);
	ent->type = CFG_INT;
	if(ent->value.vint != value) {
		ent->value.vint = value;
		store->modified = true;
	}
}

int Config_GetInt(CFGSTORE store, const char* key) {
	CFGENTRY ent = Config_GetEntry2(store, key);
	cfgTypeChecker(store, ent, CFG_INT);
	return ent->value.vint;
}

void Config_SetStr(CFGSTORE store, const char* key, const char* value) {
	CFGENTRY ent = Config_GetEntry2(store, key);
	if(ent->type == CFG_STR)
		Memory_Free((void*)ent->value.vchar);
	else
		ent->type = CFG_STR;
	ent->value.vchar = String_AllocCopy(value);
	store->modified = true;
}

const char* Config_GetStr(CFGSTORE store, const char* key) {
	CFGENTRY ent = Config_GetEntry2(store, key);
	cfgTypeChecker(store, ent, CFG_STR);
	return ent->value.vchar;
}

void Config_SetBool(CFGSTORE store, const char* key, bool value) {
	CFGENTRY ent = Config_GetEntry2(store, key);
	ent->type = CFG_BOOL;
	if(ent->value.vbool != value) {
		ent->value.vbool = value;
		store->modified = true;
	}
}

bool Config_GetBool(CFGSTORE store, const char* key) {
	CFGENTRY ent = Config_GetEntry2(store, key);
	cfgTypeChecker(store, ent, CFG_BOOL);
	return ent->value.vbool;
}

void Config_EmptyStore(CFGSTORE store) {
	CFGENTRY prev, ent = store->firstCfgEntry;

	while(ent) {
		prev = ent;
		if(ent->type == CFG_STR)
			Memory_Free((void*)ent->value.vchar);
		ent = ent->next;
		Memory_Free(prev);
	}

	store->modified = true;
	store->firstCfgEntry = NULL;
	store->lastCfgEntry = NULL;
}
