#ifndef CONFIG_H
#define CONFIG_H
enum cfgTypes {
	CFG_INVTYPE,
	CFG_BOOL,
	CFG_INT,
	CFG_INT16,
	CFG_INT8,
	CFG_STR
};

typedef struct cfgEntry {
	const char* key; // Ключ, присваиваемый записи при создании
	int type; // Тип cfg-записи
	bool readed; // Была ли осуществленна попытка чтения значения из cfg файла
	bool changed; // Отличается ли текущее значение записи от заданного стандартного
	bool haveLimits; // Применимо только для integer типов
	int limits[2]; // Минимальный и максимальный предел значений записи
	union {
		int vint;
		int8_t vint8;
		int16_t vint16;
		bool vbool;
		const char* vchar;
	} value; // Значение записи, заданное пользователем
	union {
		int32_t vint;
		int8_t vint8;
		int16_t vint16;
		bool vbool;
		const char* vchar;
	} defvalue; // Значение записи, заданное по умолчанию
	const char* commentary; // Комментарий к записи
	struct cfgEntry* next; // Следующая запись
	struct cfgStore* store; // Cfg-хранилище, которому принадлежит запись
} *CFGENTRY;

typedef struct cfgStore {
	const char* path; // Путь до cfg-файла
	bool modified; // Было ли хранилище модифицировано во время работы сервера
	CFGENTRY firstCfgEntry; // Первая запись в хранилище
	CFGENTRY lastCfgEntry; // Последняя запись в хранилище
} *CFGSTORE;


API const char* Config_TypeName(int type);
API int Config_TypeNameToInt(const char* name);
API bool Config_ToStr(CFGENTRY ent, char* value, uint8_t len);

API CFGSTORE Config_NewStore(const char* path);
API void Config_EmptyStore(CFGSTORE store);
API void Config_DestroyStore(CFGSTORE store);

API CFGENTRY Config_NewEntry(CFGSTORE store, const char* key, int type);
API CFGENTRY Config_GetEntry(CFGSTORE store, const char* key);


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

API const char* Config_GetStr(CFGSTORE store, const char* key);
API void Config_SetDefaultStr(CFGENTRY ent, const char* value);
API void Config_SetStr(CFGENTRY ent, const char* value);

API bool Config_GetBool(CFGSTORE store, const char* key);
API void Config_SetDefaultBool(CFGENTRY ent, bool value);
API void Config_SetBool(CFGENTRY ent, bool value);
#endif
