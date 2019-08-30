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
		bool  vbool;
		const char* vchar;
	} value;
	struct cfgEntry* next;
} CFGENTRY;

typedef struct cfgStore {
	const char* name;
	bool modified;
	CFGENTRY* firstCfgEntry;
} CFGSTORE;

CFGENTRY* Config_GetEntry(const char* key);

bool  Config_Load(const char* filename);
bool  Config_Save(const char* filename);

int   Config_GetInt(const char* key);
void  Config_SetInt(const char* key, int value);

const char* Config_GetStr(const char* key);
void  Config_SetStr(const char* key, const char* value);

void Config_SetBool(const char* key, bool value);
bool Config_GetBool(const char* key);

CFGENTRY* firstCfgEntry;
CFGENTRY* lastCfgEntry;
#endif
