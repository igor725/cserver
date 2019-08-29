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

		char* hkey, *hval;
		if(!(hkey = Memory_Alloc(String_Length(key) + 1, 1)))
			return false;
		String_CopyUnsafe(hkey, key);
		switch (type) {
			case CFG_STR:
				if(!(hval = Memory_Alloc(String_Length(value) + 1, 1)))
					return false;
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

	File_Close(fp);
	return true;
}

bool Config_Save(const char* filename) {
	FILE* fp = File_Open(filename, "w");
	if(!fp) {
		return false;
	}

	CFGENTRY* ptr = headCfgEntry;
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

CFGENTRY* Config_GetStruct(const char* key) {
	CFGENTRY* ptr = headCfgEntry;
	while(ptr) {
		if(String_CaselessCompare(ptr->key, key)) {
			return ptr;
		}
		if(!ptr->next)
			break;
		else
			ptr = ptr->next;
	}

	ptr = (CFGENTRY*)Memory_Alloc(1, sizeof(CFGENTRY));
	ptr->next = headCfgEntry;
	ptr->key = key;
	headCfgEntry = ptr;

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
