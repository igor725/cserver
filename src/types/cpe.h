#ifndef CPETYPES_H
#define CPETYPES_H
#include "core.h"
#include "types/list.h"

#define PCU_NONE 0x00 // Ни одно из CPE-значений игрока не изменилось
#define PCU_NAME BIT(0) // Была обновлено имя игрока, либо группа
#define PCU_MODEL BIT(1) // Была изменена модель игрока
#define PCU_SKIN BIT(2) // Был изменён скин игрока
#define PCU_ENTPROP BIT(3) // Модель игрока была повёрнута

#define MV_NONE    0x00
#define MV_COLORS  BIT(0)
#define MV_PROPS   BIT(1)
#define MV_TEXPACK BIT(2)
#define MV_WEATHER BIT(3)

/*
** Если какой-то из дефайнов ниже
** вырос, удостовериться, что
** int-типа в структуре _WorldInfo
** хватает для представления всех
** этих значений в степени двойки.
*/
#define WORLD_PROPS_COUNT 10
#define WORLD_COLORS_COUNT 5

typedef enum _EWorldColor {
	WORLD_COLOR_SKY,
	WORLD_COLOR_CLOUD,
	WORLD_COLOR_FOG,
	WORLD_COLOR_AMBIENT,
	WORLD_COLOR_DIFFUSE
} EColors;

typedef enum _EProp {
	WORLD_PROP_SIDEBLOCK,
	WORLD_PROP_EDGEBLOCK,
	WORLD_PROP_EDGELEVEL,
	WORLD_PROP_CLOUDSLEVEL,
	WORLD_PROP_FOGDIST,
	WORLD_PROP_SPDCLOUDS,
	WORLD_PROP_SPDWEATHER,
	WORLD_PROP_FADEWEATHER,
	WORLD_PROP_EXPFOG,
	WORLD_PROP_SIDEOFFSET
} EProp;

typedef enum _EWeather {
	WORLD_WEATHER_SUN,
	WORLD_WEATHER_RAIN,
	WORLD_WEATHER_SNOW
} EWeather;

typedef struct {
	cs_int16 r, g, b, a;
} Color4;

typedef struct {
	cs_int16 r, g, b;
} Color3;

typedef struct _CPEExt {
	cs_str name; // Название дополнения
	cs_int32 version; // Его версия
	cs_ulong hash; // crc32 хеш названия дополнения
	struct _CPEExt *next; // Следующее дополнение
} CPEExt;

typedef struct _CustomParticle {
	cs_byte id;
	struct TextureRec {cs_byte U1, V1, U2, V2;} rec;
	Color3 tintCol;
	cs_byte frameCount, particleCount, collideFlags;
	cs_bool fullBright;
	cs_float size, sizeVariation, spread, speed,
	gravity, baseLifetime, lifetimeVariation;
} CustomParticle;

/**
 * @brief Структура, описывающая создаваемый блок.
 * 
 */
typedef struct _BlockDef {
	cs_char name[65]; /** Название блока */
	BlockID fallback; /** Блок-замена, если клиент не поддерживает дополнение */
	cs_byte flags; /** Флаги блока */
	union {
		struct _BlockParamsExt {
			cs_byte solidity; /** Прочность блока */
			cs_byte moveSpeed; /** Скорость передвижения по блоку или внутри него */
			cs_byte topTex; /** Текстура верхней границы блока */
			cs_byte leftTex; /** Текстура левой границы блока */
			cs_byte rightTex; /** Текстура правой границы блока */
			cs_byte frontTex; /** Текстура передней границы блока */
			cs_byte backTex; /** Текстура задней границы блока */
			cs_byte bottomTex; /** Текстура нижней границы блока */
			cs_bool transmitsLight; /** Пропускает ли блок свет */
			cs_byte walkSound; /** Звук хождения по блоку */
			cs_bool fullBright; /** Действуют ли тени на блок */
			cs_byte minX, minY, minZ;
			cs_byte maxX, maxY, maxZ;
			cs_byte blockDraw; /** Тип прозрачности блока */
			cs_byte fogDensity; /** Плотность тумана внутри блока */
			cs_byte fogR, fogG, fogB; /** Цвет тумана */
		} ext; /** Расширенная структура блока */
		struct _BlockParams {
			cs_byte solidity; /** Прочность блока */
			cs_byte moveSpeed; /** Скорость передвижения по блоку или внутри него */
			cs_byte topTex; /** Текстура верхней границы блока */
			cs_byte sideTex; /** Текстура боковых границ блока */
			cs_byte bottomTex; /** Текстура нижней границы блока */
			cs_byte transmitsLight; /** Пропускает ли блок свет */
			cs_byte walkSound; /** Звук хождения по блоку */
			cs_byte fullBright; /** Действуют ли тени на блок */
			cs_byte shape; /** Высота блока */
			cs_byte blockDraw; /** Тип прозрачности блока */
			cs_byte fogDensity; /** Плотность тумана внутри блока */
			cs_byte fogR, fogG, fogB; /** Цвет тумана */
		} nonext; /** Обычная структура блока */
	} params; /** Объединение параметров блока */
} BlockDef;

typedef struct _CPEData {
	CPEExt *headExtension; // Список дополнений клиента
	cs_char appName[65]; // Название игрового клиента
	cs_char skin[65]; // Скин игрока, может быть NULL [ExtPlayerList]
	cs_char *message; // Используется для получения длинных сообщений [LongerMessages]
	BlockID heldBlock; // Выбранный игроком блок в данный момент [HeldBlock]
	cs_int8 updates; // Обновлённые значения игрока
	cs_bool hideDisplayName; // Будет ли ник игрока скрыт [ExtPlayerList]
	cs_bool pingStarted; // Начат ли процесс пингования [TwoWayPing]
	cs_int16 _extCount; // Переменная используется при получении списка дополнений
	cs_int16 model; // Текущая модель игрока [ChangeModel]
	cs_int16 group; // Текущая группа игрока [ExtPlayerList]
	cs_uint16 pingData; // Данные, цепляемые к пинг-запросу
	cs_uint32 pingTime; // Сам пинг, в миллисекундах
	cs_float pingAvgTime; // Средний пинг, в миллисекундах
	cs_uint32 _pingAvgSize; // Количество значений в текущем среднем пинга
	cs_uint64 pingStart; // Время начала пинг-запроса
	cs_int32 rotation[3]; // Вращение модели игрока в градусах [EntityProperty]
	cs_uint16 clickDist; // Расстояние клика игрока
	cs_byte cbLevel; // Уровень дополнения CustomBlocks, пока не используется
} CPEData;

typedef struct _CGroup {
	cs_int16 id;
	cs_byte rank;
	cs_char name[65];
	AListField *field;
} CGroup;

typedef struct _CPEHacks {
	cs_bool flying, noclip, speeding,
	spawnControl, tpv;
	cs_int16 jumpHeight;
} CPEHacks;

/**
 * @brief Перечисление видов прочности блока.
 * 
 */
typedef enum _EBlockSolidity {
	BDSOL_WALK, /** Игрок может проходить сквозь блок */
	BDSOL_SWIM, /** Игрок может плавать сквозь блок */
	BDSOL_SOLID /** Игрок может наступить на блок */
} EBlockSolidity;

/**
 * @brief Перечисление звуков, издаваемых блоком, когда игрок на него наступает.
 * 
 */
typedef enum _EBlockSounds {
	BDSND_NONE, /** Блок не издаёт никаких звуков */
	BDSND_WOOD, /** Блок издаёт звук дерева */
	BDSND_GRAVEL, /** Блок издаёт звук гравия */
	BDSND_GRASS, /** Блок издаёт звук травы */
	BDSND_STONE, /** Блок издаёт звук камня */
	BDSND_METAL, /** Блок издаёт металлический звук */
	BDSND_GLASS, /** Блок издаёт звук стекла */
	BDSND_WOOL, /** Блок издаёт звук шерсти */
	BDSND_SAND, /** Блок издаёт звук песка */
	BDSND_SNOW /** Блок издаёт звук снега */
} EBlockSounds;

/**
 * @brief Перечисление типов прозрачности блока.
 * 
 */
typedef enum _EBlockDrawTypes {
	BDDRW_OPAQUE, /** Непрозрачный */
	BDDRW_TRANSPARENT, /** Прозрачный (н-р стекло) */
	BDDRW_TRANSPARENT2,  /** Прозрачный, но с непрозрачными внутренними границами (н-р листва) */
	BDDRW_TRANSLUCENT, /** Прозрачный и затенённый текстурой (н-р лёд, вода) */
	BDDRW_GAS /** Полностью прозрачный (н-р воздух) */
} EBlockDrawTypes;
#endif
