#ifndef CONFIG_H
#define CONFIG_H
#include "core.h"

typedef enum _ECTypes {
	CONFIG_TYPE_BOOL,
	CONFIG_TYPE_INT32,
	CONFIG_TYPE_INT16,
	CONFIG_TYPE_INT8,
	CONFIG_TYPE_STR
} ECTypes;

typedef union _CUValue {
	cs_bool vbool;
	cs_int8 vint8;
	cs_int16 vint16;
	cs_int32 vint;
	cs_str vchar;
} CUValue;

#define CFG_MAX_LEN 128
#define CFG_FREADED BIT(0) // Была ли осуществленна попытка чтения значения из cfg файла
#define CFG_FCHANGED BIT(1) // Отличается ли текущее значение записи от заданного стандартного
#define CFG_FHAVELIMITS BIT(2) // Применимо только для integer типов

typedef struct _CEntry {
	ECTypes type; // Тип cfg-записи
	cs_byte flags; // Флаги cfg-записи
	cs_str commentary; // Комментарий к записи
	cs_str key; // Ключ, присваиваемый записи при создании
	CUValue value, defvalue; // Значение записи, заданное пользователем
	cs_int32 limits[2]; // Минимальный и максимальный предел значений записи
	struct _CEntry *next; // Следующая запись
	struct _CStore *store; // Cfg-хранилище, которому принадлежит запись
} CEntry;

typedef struct _CStore {
	cs_str path; // Путь до cfg-файла
	cs_bool modified; // Было ли хранилище модифицировано во время работы сервера
	cs_int32 etype; // Тип произошедшей ошибки ET_SYS/ET_SERVER (см. объявления в cserror.h)
	cs_int32 ecode; // Код произошедшей ошибки (объявления также в cserror.h)
	cs_int32 eline; // Номер строки в файле, на которой произошла ошибка
	CEntry *firstCfgEntry, // Первая запись в хранилище
	*lastCfgEntry; // Последняя запись в хранилище
} CStore;

API cs_str Config_TypeName(ECTypes type);
API ECTypes Config_TypeNameToEnum(cs_str name);
API cs_byte Config_ToStr(CEntry *ent, cs_char *value, cs_byte len);
API void Config_PrintError(CStore *store);

API CStore *Config_NewStore(cs_str path);
API void Config_EmptyStore(CStore *store);
API void Config_DestroyStore(CStore *store);

API CEntry *Config_NewEntry(CStore *store, cs_str key, cs_int32 type);
API CEntry *Config_GetEntry(CStore *store, cs_str key);
API CEntry *Config_CheckEntry(CStore *store, cs_str key);

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
