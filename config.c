#include "core.h"
#include "stdio.h"
#include "string.h"
#include "config.h"
#include "stdlib.h"

//TODO: Оптимизировать фуникции Config_Load и Config_Save

bool Config_Load(const char* filename) {
	FILE* fp = fopen(filename, "r");
	if(fp == NULL)
		return false;

	int count = 0;
	char key[128] = {0};
	char value[128] = {0};
	int  type;

	char ch = fgetc(fp);
	while(ch != EOF) {
		memset(key, 0, 128);
		memset(value, 0, 128);
		do {
			key[count] = ch;
			ch = fgetc(fp);
			count++;
		} while(ch != '=' && ch != EOF);
		if(ch == EOF) return false;

		count = 0;
		type = fgetc(fp);
		ch = fgetc(fp);
		do {
			if(ch != '\r')
				value[count] = ch;
			count++;
			ch = fgetc(fp);
		} while(ch != '\n' && ch != EOF);
		count = 0;

		char* hkey = malloc(strlen(key) + 1);
		strcpy(hkey, key);
		if(type == CFG_STR) {
			char* hval = malloc(strlen(value) + 1);
			strcpy(hval, value);
			Config_SetStr(hkey, hval);
		} else if(type == CFG_INT) {
			Config_SetInt(hkey, atoi(value));
		} else {
			return false;
		}
		ch = fgetc(fp);
	}

	fclose(fp);
	return true;
}

bool Config_Save(const char* filename) {
	FILE* fp = fopen(filename, "w");
	if(fp == NULL)
		return false;

	CFGENTRY* ptr = firstCfgEntry;
	while(ptr != NULL) {
		fwrite(ptr->key, strlen(ptr->key), 1, fp);
		switch (ptr->type) {
			case CFG_STR:
				fprintf(fp, "=s%s\n", (char*)ptr->value.vchar);
				break;
			case CFG_INT:
				fprintf(fp, "=i%d\n", (int)ptr->value.vint);
				break;
		}
		ptr = ptr->next;
	}

	fclose(fp);
	return false;
}

CFGENTRY* Config_AllocEntry() {
	CFGENTRY* ptr = malloc(sizeof(struct cfgEntry));
	memset(ptr, 0, sizeof(struct cfgEntry));
	return ptr;
}

CFGENTRY* Config_GetStruct(char* key) {
	CFGENTRY* ptr = firstCfgEntry;
	while(ptr != NULL) {
		if(stricmp(ptr->key, key) == 0) {
			return ptr;
		}
		if(ptr->next == NULL) {
			break;
		} else
			ptr = ptr->next;
	}

	if(ptr == NULL) {
		ptr = Config_AllocEntry();
		firstCfgEntry = ptr;
		ptr->key = key;
	} else {
		ptr->next = Config_AllocEntry();
		ptr = ptr->next;
		ptr->key = key;
	}

	return ptr;
}

void Config_SetInt(char* key, int value) {
	CFGENTRY* ent = Config_GetStruct(key);
	ent->value.vint = value;
}

int Config_GetInt(char* key) {
	CFGENTRY* ent = Config_GetStruct(key);
	return ent->value.vint;
}

void Config_SetStr(char* key, char* value) {
	CFGENTRY* ent = Config_GetStruct(key);
	ent->value.vchar = value;
}

char* Config_GetStr(char* key) {
	CFGENTRY* ent = Config_GetStruct(key);
	return ent->value.vchar;
}
