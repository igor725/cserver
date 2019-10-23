#ifndef CONFIG_H
#define CONFIG_H
enum cfgTypes {
	CFG_INVTYPE = -1,
	CFG_BOOL  = 'b',
	CFG_INT   = 'i',
	CFG_INT16 = 'h',
	CFG_INT8  = 'c',
	CFG_STR   = 's'
};

typedef struct cfgEntry {
	const char* key;
	int type;
	bool changed;
	bool haveLimits;
	int limits[2];
	union {
		int vint;
		int8_t vint8;
		int16_t vint16;
		bool vbool;
		const char* vchar;
	} value;
	union {
		int32_t vint;
		int8_t vint8;
		int16_t vint16;
		bool vbool;
		const char* vchar;
	} defvalue;
	const char* commentary;
	struct cfgEntry* next;
	struct cfgStore* store;
} *CFGENTRY;

typedef struct cfgStore {
	const char* path;
	bool modified;
	CFGENTRY firstCfgEntry;
	CFGENTRY lastCfgEntry;
} *CFGSTORE;

CFGENTRY Config_GetEntry(CFGSTORE store, const char* key);

API CFGSTORE Config_Create(const char* filename);
API CFGENTRY Config_NewEntry(CFGSTORE store, const char* key);
API void Config_EmptyStore(CFGSTORE store);
API void Config_DestroyStore(CFGSTORE store);
API const char* Config_TypeName(int type);
API int Config_TypeNameToInt(const char* name);

API bool Config_Load(CFGSTORE store);
API bool Config_Save(CFGSTORE store);

API void Config_SetComment(CFGENTRY ent, const char* commentary);
API void Config_SetLimit(CFGENTRY ent, int min, int max);

API int32_t Config_GetInt(CFGSTORE store, const char* key);
API uint32_t Config_GetUInt(CFGSTORE store, const char* key);
API int16_t Config_GetInt16(CFGSTORE store, const char* key);
API uint16_t Config_GetUInt16(CFGSTORE store, const char* key);
API int8_t Config_GetInt8(CFGSTORE store, const char* key);
API uint8_t Config_GetUInt8(CFGSTORE store, const char* key);
API void Config_SetDefaultInt(CFGENTRY ent, int value);
API void Config_SetDefaultInt8(CFGENTRY ent, int8_t value);
API void Config_SetDefaultInt16(CFGENTRY ent, int16_t value);
API void Config_SetInt(CFGENTRY ent, int32_t value);
API void Config_SetInt16(CFGENTRY ent, int16_t value);
API void Config_SetInt8(CFGENTRY ent, int8_t value);
API void Config_SetIntByKey(CFGSTORE store, const char* key, int value);

API const char* Config_GetStr(CFGSTORE store, const char* key);
API void Config_SetDefaultStr(CFGENTRY ent, const char* value);
API void Config_SetStr(CFGENTRY ent, const char* value);
API void Config_SetStrByKey(CFGSTORE store, const char* key, const char* value);

API bool Config_GetBool(CFGSTORE store, const char* key);
API void Config_SetDefaultBool(CFGENTRY ent, bool value);
API void Config_SetBool(CFGENTRY ent, bool value);
API void Config_SetBoolByKey(CFGSTORE store, const char* key, bool value);
#endif
