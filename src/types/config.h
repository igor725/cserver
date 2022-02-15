#ifndef CONFIGTYPES_H
#define CONFIGTYPES_H
#include "core.h"

typedef enum _ECError {
	CONFIG_ERROR_SUCCESS = 0,
	CONFIG_ERROR_INTERNAL,
	CONFIG_ERROR_IOFAIL,
	CONFIG_ERROR_PARSE,

	CONFIG_MAX_ERROR
} ECError;

typedef enum _ECExtra {
	CONFIG_EXTRA_NOINFO = 0,
	CONFIG_EXTRA_IO_LINEASERROR,
	CONFIG_EXTRA_IO_FRENAME,
	CONFIG_EXTRA_PARSE_NOENTRY,
	CONFIG_EXTRA_PARSE_LINEFORMAT,
	CONFIG_EXTRA_PARSE_NUMBER,
	CONFIG_EXTRA_PARSE_END,

	CONFIG_MAX_EXTRA
} ECExtra;

typedef enum _ECTypes {
	CONFIG_TYPE_BOOL,
	CONFIG_TYPE_INT32,
	CONFIG_TYPE_INT16,
	CONFIG_TYPE_INT8,
	CONFIG_TYPE_STR,

	CONFIG_MAX_TYPE
} ECTypes;

typedef union _CUValue {
	cs_bool vbool;
	cs_int8 vint8;
	cs_int16 vint16;
	cs_int32 vint;
	cs_str vchar;
} CUValue;

#define CFG_MAX_LEN 128 // Максимальная длина строки в cfg файле
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
	struct _CError {
		ECError code;
		ECExtra extra;
		cs_int32 line;
	} error; // Информация о ошибках
	CEntry *firstCfgEntry, // Первая запись в хранилище
	*lastCfgEntry; // Последняя запись в хранилище
} CStore;
#endif
