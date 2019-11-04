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
	cs_int32 type; // Тип cfg-записи
	cs_bool readed; // Была ли осуществленна попытка чтения значения из cfg файла
	cs_bool changed; // Отличается ли текущее значение записи от заданного стандартного
	cs_bool haveLimits; // Применимо только для integer типов
	cs_int32 limits[2]; // Минимальный и максимальный предел значений записи
	union {
		cs_int32 vint;
		cs_int8 vint8;
		cs_int16 vint16;
		cs_bool vbool;
		const char* vchar;
	} value; // Значение записи, заданное пользователем
	union {
		cs_int32 vint;
		cs_int8 vint8;
		cs_int16 vint16;
		cs_bool vbool;
		const char* vchar;
	} defvalue; // Значение записи, заданное по умолчанию
	const char* commentary; // Комментарий к записи
	struct cfgEntry* next; // Следующая запись
	struct cfgStore* store; // Cfg-хранилище, которому принадлежит запись
} *CFGEntry;

typedef struct cfgStore {
	const char* path; // Путь до cfg-файла
	cs_bool modified; // Было ли хранилище модифицировано во время работы сервера
	cs_int32 etype; // Тип произошедшей ошибки ET_SYS/ET_SERVER (см. объявления в error.h)
	cs_int32 ecode; // Код произошедшей ошибки (объявления также в error.h)
	cs_int32 eline; // Номер строки в файле, на которой произошла ошибка
	CFGEntry firstCfgEntry; // Первая запись в хранилище
	CFGEntry lastCfgEntry; // Последняя запись в хранилище
} *CFGStore;


API const char* Config_TypeName(cs_int32 type);
API cs_int32 Config_TypeNameToInt(const char* name);
API cs_bool Config_ToStr(CFGEntry ent, char* value, cs_uint8 len);
API void Config_PrintError(CFGStore store);

API CFGStore Config_NewStore(const char* path);
API void Config_EmptyStore(CFGStore store);
API void Config_DestroyStore(CFGStore store);

API CFGEntry Config_NewEntry(CFGStore store, const char* key, cs_int32 type);
API CFGEntry Config_GetEntry(CFGStore store, const char* key);


API cs_bool Config_Load(CFGStore store);
API cs_bool Config_Save(CFGStore store);

API void Config_SetComment(CFGEntry ent, const char* commentary);
API void Config_SetLimit(CFGEntry ent, cs_int32 min, cs_int32 max);

API cs_int32 Config_GetInt(CFGStore store, const char* key);
API cs_uint32 Config_GetUInt(CFGStore store, const char* key);
API cs_int16 Config_GetInt16(CFGStore store, const char* key);
API cs_uint16 Config_GetUInt16(CFGStore store, const char* key);
API cs_int8 Config_GetInt8(CFGStore store, const char* key);
API cs_uint8 Config_GetUInt8(CFGStore store, const char* key);
API void Config_SetDefaultInt(CFGEntry ent, cs_int32 value);
API void Config_SetDefaultInt8(CFGEntry ent, cs_int8 value);
API void Config_SetDefaultInt16(CFGEntry ent, cs_int16 value);
API void Config_SetInt(CFGEntry ent, cs_int32 value);
API void Config_SetInt16(CFGEntry ent, cs_int16 value);
API void Config_SetInt8(CFGEntry ent, cs_int8 value);

API const char* Config_GetStr(CFGStore store, const char* key);
API void Config_SetDefaultStr(CFGEntry ent, const char* value);
API void Config_SetStr(CFGEntry ent, const char* value);

API cs_bool Config_GetBool(CFGStore store, const char* key);
API void Config_SetDefaultBool(CFGEntry ent, cs_bool value);
API void Config_SetBool(CFGEntry ent, cs_bool value);
#endif
