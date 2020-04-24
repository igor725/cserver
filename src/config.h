#ifndef CONFIG_H
#define CONFIG_H
enum {
	CFG_INVTYPE,
	CFG_BOOL,
	CFG_INT32,
	CFG_INT16,
	CFG_INT8,
	CFG_STR
};

typedef struct _CEntry {
	cs_str key; // Ключ, присваиваемый записи при создании
	cs_int32 type; // Тип cfg-записи
	cs_bool readed; // Была ли осуществленна попытка чтения значения из cfg файла
	cs_bool changed; // Отличается ли текущее значение записи от заданного стандартного
	cs_bool haveLimits; // Применимо только для integer типов
	cs_int32 limits[2]; // Минимальный и максимальный предел значений записи
	union {
		cs_bool vbool;
		cs_int8 vint8;
		cs_int16 vint16;
		cs_int32 vint;
		cs_str vchar;
	} value; // Значение записи, заданное пользователем
	union {
		cs_bool vbool;
		cs_int8 vint8;
		cs_int16 vint16;
		cs_int32 vint;
		cs_str vchar;
	} defvalue; // Значение записи, заданное по умолчанию
	cs_str commentary; // Комментарий к записи
	struct _CEntry *next; // Следующая запись
	struct _CStore *store; // Cfg-хранилище, которому принадлежит запись
} CEntry;

typedef struct _CStore {
	cs_str path; // Путь до cfg-файла
	cs_bool modified; // Было ли хранилище модифицировано во время работы сервера
	cs_int32 etype; // Тип произошедшей ошибки ET_SYS/ET_SERVER (см. объявления в error.h)
	cs_int32 ecode; // Код произошедшей ошибки (объявления также в error.h)
	cs_int32 eline; // Номер строки в файле, на которой произошла ошибка
	CEntry *firstCfgEntry; // Первая запись в хранилище
	CEntry *lastCfgEntry; // Последняя запись в хранилище
} CStore;


API cs_str Config_TypeName(cs_int32 type);
API cs_int32 Config_TypeNameToInt(cs_str name);
API cs_bool Config_ToStr(CEntry *ent, cs_char *value, cs_byte len);
API void Config_PrintError(CStore *store);

API CStore *Config_NewStore(cs_str path);
API void Config_EmptyStore(CStore *store);
API void Config_DestroyStore(CStore *store);

API CEntry *Config_NewEntry(CStore *store, cs_str key, cs_int32 type);
API CEntry *Config_GetEntry(CStore *store, cs_str key);

API cs_bool Config_Load(CStore *store);
API cs_bool Config_Save(CStore *store);

API void Config_SetComment(CEntry *ent, cs_str commentary);
API void Config_SetLimit(CEntry *ent, cs_int32 min, cs_int32 max);

API cs_int32 Config_GetInt32(CEntry *ent);
API cs_int32 Config_GetInt32ByKey(CStore *store, cs_str key);
API cs_int16 Config_GetInt16(CEntry *ent);
API cs_int16 Config_GetInt16ByKey(CStore *store, cs_str key);
API cs_int8 Config_GetInt8(CEntry *ent);
API cs_int8 Config_GetInt8ByKey(CStore *store, cs_str key);

API void Config_SetDefaultInt32(CEntry *ent, cs_int32 value);
API void Config_SetDefaultInt16(CEntry *ent, cs_int16 value);
API void Config_SetDefaultInt8(CEntry *ent, cs_int8 value);
API void Config_SetInt32(CEntry *ent, cs_int32 value);
API void Config_SetInt16(CEntry *ent, cs_int16 value);
API void Config_SetInt8(CEntry *ent, cs_int8 value);

API cs_str Config_GetStr(CEntry *ent);
API cs_str Config_GetStrByKey(CStore *store, cs_str key);
API void Config_SetDefaultStr(CEntry *ent, cs_str value);
API void Config_SetStr(CEntry *ent, cs_str value);

API cs_bool Config_GetBool(CEntry *ent);
API cs_bool Config_GetBoolByKey(CStore *store, cs_str key);
API void Config_SetDefaultBool(CEntry *ent, cs_bool value);
API void Config_SetBool(CEntry *ent, cs_bool value);
#endif // CONFIG_H
