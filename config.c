#include "core.h"
#include "error.h"
#include "config.h"

bool Config_Load(const char* filename) {
	FILE* fp = fopen(filename, "r");
	if(!fp) {
		Error_Set(ET_SYS, GetLastError());
		return false;
	}

	int type;
	int count = 0;
	int ch = fgetc(fp);
	char key[128] = {0};
	char value[128] = {0};

	while(!feof(fp)) {
		Memory_Fill(key, 128, 0);
		Memory_Fill(value, 128, 0);
		do {
			key[count] = ch;
			ch = fgetc(fp);
			count++;
		} while(ch != '=' && !feof(fp));
		if(feof(fp)) return false;

		count = 0;
		type = fgetc(fp);
		ch = fgetc(fp);
		do {
			if(ch != '\r')
				value[count] = ch;
			count++;
			ch = fgetc(fp);
		} while(ch != '\n' && !feof(fp));
		count = 0;

		char* hkey = Memory_Alloc(String_Length(key) + 1, 1);
		char* hval;
		String_CopyUnsafe(hkey, key);
		switch (type) {
			case CFG_STR:
				hval = Memory_Alloc(String_Length(value) + 1, 1);
				String_CopyUnsafe(hval, value);
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

	fclose(fp);
	return true;
}

bool Config_Save(const char* filename) {
	FILE* fp = fopen(filename, "w");
	if(!fp) {
		Error_Set(ET_SYS, GetLastError());
		return false;
	}

	CFGENTRY* ptr = firstCfgEntry;
	while(ptr) {
		fwrite(ptr->key, String_Length(ptr->key), 1, fp);
		switch (ptr->type) {
			case CFG_STR:
				fprintf(fp, "=s%s\n", (char*)ptr->value.vchar);
				break;
			case CFG_INT:
				fprintf(fp, "=i%d\n", (int)ptr->value.vint);
				break;
			case CFG_BOOL:
				fprintf(fp, "=b%s\n", ptr->value.vbool ? "True" : "False");
				break;
			default:
				fwrite("=sUnknown value\n", 16, 1, fp);
				break;
		}
		ptr = ptr->next;
	}

	fclose(fp);
	return false;
}

CFGENTRY* Config_GetStruct(const char* key) {
	CFGENTRY* ptr = firstCfgEntry;
	while(ptr) {
		if(String_CaselessCompare(ptr->key, key)) {
			return ptr;
		}
		if(!ptr->next)
			break;
		else
			ptr = ptr->next;
	}

	if(!ptr) {
		ptr = (CFGENTRY*)Memory_Alloc(1, sizeof(CFGENTRY));
		firstCfgEntry = ptr;
		ptr->key = key;
	} else {
		ptr->next = (CFGENTRY*)Memory_Alloc(1, sizeof(CFGENTRY));
		ptr = ptr->next;
		ptr->key = key;
	}

	return ptr;
}

void Config_SetInt(const char* key, int value) {
	CFGENTRY* ent = Config_GetStruct(key);
	ent->value.vint = value;
	ent->type = CFG_INT;
}

int Config_GetInt(const char* key) {
	CFGENTRY* ent = Config_GetStruct(key);
	return ent->value.vint;
}

void Config_SetStr(const char* key, char* value) {
	CFGENTRY* ent = Config_GetStruct(key);
	ent->value.vchar = value;
	ent->type = CFG_STR;
}

char* Config_GetStr(const char* key) {
	CFGENTRY* ent = Config_GetStruct(key);
	return ent->value.vchar;
}

void Config_SetBool(const char* key, bool value) {
	CFGENTRY* ent = Config_GetStruct(key);
	ent->value.vbool = value;
	ent->type = CFG_BOOL;
}

bool Config_GetBool(const char* key) {
	CFGENTRY* ent = Config_GetStruct(key);
	return ent->value.vbool;
}
