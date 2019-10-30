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
	int32_t type; // Тип cfg-записи
	bool readed; // Была ли осуществленна попытка чтения значения из cfg файла
	bool changed; // Отличается ли текущее значение записи от заданного стандартного
	bool haveLimits; // Применимо только для integer типов
	int32_t limits[2]; // Минимальный и максимальный предел значений записи
	union {
		int32_t vint;
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
} *CFGEntry;

typedef struct cfgStore {
	const char* path; // Путь до cfg-файла
	bool modified; // Было ли хранилище модифицировано во время работы сервера
	int32_t etype; // Тип произошедшей ошибки ET_SYS/ET_SERVER (см. объявления в error.h)
	int32_t ecode; // Код произошедшей ошибки (объявления также в error.h)
	int32_t eline; // Номер строки в файле, на которой произошла ошибка
	CFGEntry firstCfgEntry; // Первая запись в хранилище
	CFGEntry lastCfgEntry; // Последняя запись в хранилище
} *CFGStore;


API const char* Config_TypeName(int32_t type);
API int32_t Config_TypeNameToInt(const char* name);
API bool Config_ToStr(CFGEntry ent, char* value, uint8_t len);
API void Config_PrintError(CFGStore store);

API CFGStore Config_NewStore(const char* path);
API void Config_EmptyStore(CFGStore store);
API void Config_DestroyStore(CFGStore store);

API CFGEntry Config_NewEntry(CFGStore store, const char* key, int32_t type);
API CFGEntry Config_GetEntry(CFGStore store, const char* key);


API bool Config_Load(CFGStore store);
API bool Config_Save(CFGStore store);

API void Config_SetComment(CFGEntry ent, const char* commentary);
API void Config_SetLimit(CFGEntry ent, int32_t min, int32_t max);

API int32_t Config_GetInt(CFGStore store, const char* key);
API uint32_t Config_GetUInt(CFGStore store, const char* key);
API int16_t Config_GetInt16(CFGStore store, const char* key);
API uint16_t Config_GetUInt16(CFGStore store, const char* key);
API int8_t Config_GetInt8(CFGStore store, const char* key);
API uint8_t Config_GetUInt8(CFGStore store, const char* key);
API void Config_SetDefaultInt(CFGEntry ent, int32_t value);
API void Config_SetDefaultInt8(CFGEntry ent, int8_t value);
API void Config_SetDefaultInt16(CFGEntry ent, int16_t value);
API void Config_SetInt(CFGEntry ent, int32_t value);
API void Config_SetInt16(CFGEntry ent, int16_t value);
API void Config_SetInt8(CFGEntry ent, int8_t value);

API const char* Config_GetStr(CFGStore store, const char* key);
API void Config_SetDefaultStr(CFGEntry ent, const char* value);
API void Config_SetStr(CFGEntry ent, const char* value);

API bool Config_GetBool(CFGStore store, const char* key);
API void Config_SetDefaultBool(CFGEntry ent, bool value);
API void Config_SetBool(CFGEntry ent, bool value);
#endif
