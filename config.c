#include "core.h"
#include "config.h"

bool Config_Load(const char* filename) {
	FILE* fp = File_Open(filename, "r");
	if(!fp)
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
			return false;
		}
		count = 0;

		const char* hkey, *hval;
		if(!(hkey = String_AllocCopy(key)))
			return false;

		switch (type) {
			case CFG_STR:
				if(!(hval = String_AllocCopy(value)))
					return false;
				Config_SetStr(hkey, hval);
				break;
			case CFG_INT:
				Config_SetInt(hkey, atoi(value));
				break;
			case CFG_BOOL:
				Config_SetBool(hkey, String_Compare(value, "True"));
				break;
			default:
				Error_Set(ET_SERVER, EC_CFGTYPE);
				return false;
		}
		ch = fgetc(fp);
	}

	File_Close(fp);
	return true;
}

bool Config_Save(const char* filename) {
	FILE* fp = File_Open(filename, "w");
	if(!fp) {
		return false;
	}

	CFGENTRY* ptr = firstCfgEntry;
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

CFGENTRY* Config_GetEntry(const char* key) {
	CFGENTRY* ent = firstCfgEntry;

	while(ent) {
		if(String_CaselessCompare(ent->key, key)) {
			return ent;
		}
		ent = ent->next;
	}

	return ent;
}

CFGENTRY* Config_GetEntry2(const char* key) {
	CFGENTRY* ent = Config_GetEntry(key);

	if(!ent) {
		ent = (CFGENTRY*)Memory_Alloc(1, sizeof(CFGENTRY));
		ent->key = String_AllocCopy(key);
		if(firstCfgEntry) {
			lastCfgEntry->next = ent;
			lastCfgEntry = ent;
		} else {
			lastCfgEntry = ent;
			firstCfgEntry = ent;
		}
	}

	return ent;
}

void Config_SetInt(const char* key, int value) {
	CFGENTRY* ent = Config_GetEntry2(key);
	ent->value.vint = value;
	ent->type = CFG_INT;
}

int Config_GetInt(const char* key) {
	CFGENTRY* ent = Config_GetEntry2(key);
	return ent->value.vint;
}

void Config_SetStr(const char* key, const char* value) {
	CFGENTRY* ent = Config_GetEntry2(key);
	if(ent->type == CFG_STR)
		free((void*)ent->value.vchar);
	else
		ent->type = CFG_STR;
	ent->value.vchar = String_AllocCopy(value);
}

const char* Config_GetStr(const char* key) {
	CFGENTRY* ent = Config_GetEntry2(key);
	return ent->value.vchar;
}

void Config_SetBool(const char* key, bool value) {
	CFGENTRY* ent = Config_GetEntry2(key);
	ent->value.vbool = value;
	ent->type = CFG_BOOL;
}

bool Config_GetBool(const char* key) {
	CFGENTRY* ent = Config_GetEntry2(key);
	return ent->value.vbool;
}
