#ifndef CONFIG_H
#define CONFIG_H

#define CFG_INT 105
#define CFG_STR 115

typedef struct cfgEntry {
	char*            key;
	int              type;
	union {
		int   vint;
		char* vchar;
	} value;
	struct cfgEntry* next;
} CFGENTRY;

bool  Config_Load(const char* filename);
bool  Config_Save(const char* filename);

int   Config_GetInt(char* key);
void  Config_SetInt(char* key, int value);

char* Config_GetStr(char* key);
void  Config_SetStr(char* key, char* value);

CFGENTRY* firstCfgEntry;
#endif
