#include "core.h"
#include "config.h"

CFGSTORE *Config_Create(const char *filename) {
	CFGSTORE *store = Memory_Alloc(1, sizeof(CFGSTORE));
	store->path = String_AllocCopy(filename);
	return store;
}

bool Config_Load(CFGSTORE *store) {
	FILE *fp;
	if(!(fp = File_Open(store->path, "r")))
		return false;

	int type;
	int count = 0;
	int ch = fgetc(fp);
	char key[128] = {0};
	char value[128] = {0};

	while(!feof(fp)) {
		while(ch == '\n') {
			ch = fgetc(fp);
		}
		Memory_Fill(key, 128, 0);
		Memory_Fill(value, 128, 0);

		do {
			if(ch != '\n' && ch != '\r' && ch != ' ') {
				key[count] = (char)ch;
				count++;
			}
			ch = fgetc(fp);
		} while(ch != '=' && !feof(fp));

		if(feof(fp)) {
			Error_Set(ET_SERVER, EC_CFGEND);
			return false;
		}

		count = 0;
		type = fgetc(fp);

		while((ch = fgetc(fp)) != EOF && ch != '\n') {
			if(ch != '\r') {
				value[count] = (char)ch;
				count++;
			}
		}

		if(count < 1) {
			Error_Set(ET_SERVER, EC_CFGEND);
			File_Close(fp);
			return false;
		}

		count = 0;
		const char *hkey, *hval;

		if(!(hkey = String_AllocCopy(key))) {
			File_Close(fp);
			return false;
		}

		switch (type) {
			case CFG_STR:
				if(!(hval = String_AllocCopy(value))) {
					File_Close(fp);
					return false;
				}
				Config_SetStr(store, hkey, hval);
				break;
			case CFG_INT:
				Config_SetInt(store, hkey, atoi(value));
				break;
			case CFG_BOOL:
				Config_SetBool(store, hkey, String_Compare(value, "True"));
				break;
			default:
				Error_Set(ET_SERVER, EC_CFGTYPE);
				File_Close(fp);
				return false;
		}

		ch = fgetc(fp);
	}

	File_Close(fp);
	return true;
}

bool Config_Save(CFGSTORE *store) {
	if(!store->modified)
		return true;

	FILE *fp;
	if(!(fp = File_Open(store->path, "w")))
		return false;

	CFGENTRY *ptr = store->firstCfgEntry;
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
	return true;
}

CFGENTRY *Config_GetEntry(CFGSTORE *store, const char* key) {
	CFGENTRY *ent = store->firstCfgEntry;

	while(ent) {
		if(String_CaselessCompare(ent->key, key)) {
			return ent;
		}
		ent = ent->next;
	}

	return ent;
}

CFGENTRY *Config_GetEntry2(CFGSTORE *store, const char *key) {
	CFGENTRY *ent = Config_GetEntry(store, key);

	if(!ent) {
		ent = (CFGENTRY*)Memory_Alloc(1, sizeof(CFGENTRY));
		ent->key = String_AllocCopy(key);

		if(store->firstCfgEntry)
			store->lastCfgEntry->next = ent;
		else
			store->firstCfgEntry = ent;

		store->lastCfgEntry = ent;
	}

	return ent;
}

void Config_SetInt(CFGSTORE *store, const char *key, int value) {
	CFGENTRY *ent = Config_GetEntry2(store, key);
	if(ent->value.vint != value) {
		ent->value.vint = value;
		store->modified = true;
		ent->type = CFG_INT;
	}
}

int Config_GetInt(CFGSTORE *store, const char *key) {
	CFGENTRY *ent = Config_GetEntry2(store, key);
	return ent->value.vint;
}

void Config_SetStr(CFGSTORE *store, const char *key, const char *value) {
	CFGENTRY *ent = Config_GetEntry2(store, key);
	if(ent->type == CFG_STR)
		free((void*)ent->value.vchar);
	else
		ent->type = CFG_STR;
	ent->value.vchar = String_AllocCopy(value);
	store->modified = true;
}

const char *Config_GetStr(CFGSTORE *store, const char *key) {
	CFGENTRY *ent = Config_GetEntry2(store, key);
	return ent->value.vchar;
}

void Config_SetBool(CFGSTORE *store, const char *key, bool value) {
	CFGENTRY *ent = Config_GetEntry2(store, key);
	if(ent->value.vbool != value) {
		ent->value.vbool = value;
		store->modified = true;
		ent->type = CFG_BOOL;
	}
}

bool Config_GetBool(CFGSTORE *store, const char *key) {
	CFGENTRY *ent = Config_GetEntry2(store, key);
	return ent->value.vbool;
}


void Config_EmptyStore(CFGSTORE *store) {
	CFGENTRY *prev, *ent = store->firstCfgEntry;
	if(!ent) return;

	while(ent) {
		prev = ent;
		if(ent->type == CFG_STR) {
			free((void*)ent->value.vchar);
		}
		ent = ent->next;
		free(prev);
	}

	store->modified = true;
	store->firstCfgEntry = NULL;
	store->lastCfgEntry = NULL;
}
