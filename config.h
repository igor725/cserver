#ifndef CONFIG_H
#define CONFIG_H
enum cfgTypes {
	CFG_BOOL = 'b',
	CFG_INT  = 'i',
	CFG_STR  = 's'
};

typedef struct cfgEntry {
	const char*      key;
	int              type;
	union {
		int   vint;
		char* vchar;
		bool  vbool;
	} value;
	struct cfgEntry* next;
} CFGENTRY;

bool  Config_Load(const char* filename);
bool  Config_Save(const char* filename);

int   Config_GetInt(const char* key);
void  Config_SetInt(const char* key, int value);

char* Config_GetStr(const char* key);
void  Config_SetStr(const char* key, char* value);

void Config_SetBool(const char* key, bool value);
bool Config_GetBool(const char* key);

CFGENTRY* firstCfgEntry;
#endif
