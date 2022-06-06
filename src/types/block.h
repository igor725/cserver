#ifndef BLOCKTYPES_H
#define BLOCKTYPES_H
#include "core.h"
#include "types/world.h"

/**
 * @name BDF_*
 * 
 * Флаги для динамических блоков.
 * 
 */
/* @{ */
#define BDF_EXTENDED   BIT(0) /** Блок использует расширенную спецификацию */
#define BDF_RESERVED   BIT(1) /** Зарезервированный флаг, пока не используется */
#define BDF_UPDATED    BIT(2) /** Блок был разослан клиентам */
#define BDF_UNDEFINED  BIT(3) /** Блок удалён */
/* @} */

/**
 * @brief Перечисление граней блока.
 * 
 */
typedef enum _EBlockFace {
	BLOCK_FACE_AWAY_X, /** Удалено от оси X */
	BLOCK_FACE_TOWARDS_X, /** В направлении оси X */
	BLOCK_FACE_AWAY_Y, /** Удалено от начала координат Y (верхняя грань) */
	BLOCK_FACE_TOWARDS_Y, /** В направлении начала координат Y (нижняя грань) */
	BLOCK_FACE_AWAY_Z, /** Удалено от оси Z */
	BLOCK_FACE_TOWARDS_Z /** В направлении оси Z */
} EBlockFace;

/**
 * @brief Перечисление типов блоков.
 * 
 */
typedef enum _EBlockIDs {
	BLOCK_AIR = 0, /** Воздух */
	BLOCK_STONE = 1, /** Камень */
	BLOCK_GRASS = 2, /** Трава */
	BLOCK_DIRT = 3, /** Земля */
	BLOCK_COBBLE = 4, /** Булыжник */
	BLOCK_WOOD = 5, /** Доски */
	BLOCK_SAPLING = 6, /** Росток */
	BLOCK_BEDROCK = 7, /** Бедрок */
	BLOCK_WATER = 8, /** Текущая вода */
	BLOCK_WATER_STILL = 9, /** Стоячая вода */
	BLOCK_LAVA = 10, /** Текущая лава */
	BLOCK_LAVA_STILL = 11, /** Стоячая лава */
	BLOCK_SAND = 12, /** Песок */
	BLOCK_GRAVEL = 13, /** Гравий */
	BLOCK_GOLD_ORE = 14, /** Золотая руда */
	BLOCK_IRON_ORE = 15, /** Железная руда */
	BLOCK_COAL_ORE = 16, /** Угольная руда */
	BLOCK_LOG = 17, /** Дерево */
	BLOCK_LEAVES = 18, /** Листва */
	BLOCK_SPONGE = 19, /** Губка */
	BLOCK_GLASS = 20, /** Стекло */
	BLOCK_RED = 21, /** Красная шерсть */
	BLOCK_ORANGE = 22, /** Оранжевая шерсть */
	BLOCK_YELLOW = 23, /** Желтая шерсть */
	BLOCK_LIME = 24, /** Лаймовая шерсть */
	BLOCK_GREEN = 25, /** Зелёная шерсть */
	BLOCK_TEAL = 26, /** Голубо-зелёная шерсть */
	BLOCK_AQUA = 27, /** Шерсть цвета морской воды */
	BLOCK_CYAN = 28, /** Голубая шерсть */
	BLOCK_BLUE = 29, /** Синяя шерсть */
	BLOCK_INDIGO = 30, /** Шерсть цвета индиго */
	BLOCK_VIOLET = 31, /** Фиолетовая шерсть */
	BLOCK_MAGENTA = 32, /** Пурпурная шерсть */
	BLOCK_PINK = 33, /** Розовая шерсть */
	BLOCK_BLACK = 34, /** Чёрная шерсть */
	BLOCK_GRAY = 35, /** Серая шерсть */
	BLOCK_WHITE = 36, /** Белая шерсть */
	BLOCK_DANDELION = 37, /** Одуванчик */
	BLOCK_ROSE = 38, /** Роза */
	BLOCK_BROWN_SHROOM = 39, /** Коричневый гриб */
	BLOCK_RED_SHROOM = 40, /** Красный гриб */
	BLOCK_GOLD = 41, /** Золото */
	BLOCK_IRON = 42, /** Железо */
	BLOCK_DOUBLE_SLAB = 43, /** Двойная плита */
	BLOCK_SLAB = 44, /** Одиночная плита */
	BLOCK_BRICK = 45, /** Кирпич */
	BLOCK_TNT = 46, /** Динамит */
	BLOCK_BOOKSHELF = 47, /** Книжная полка */
	BLOCK_MOSSY_ROCKS = 48, /** Замшелый булыжник */
	BLOCK_OBSIDIAN = 49, /** Обсидиан */

	/** Блоки из CPE расширения CustomBlocks */
	BLOCK_COBBLESLAB = 50,
	BLOCK_ROPE = 51,
	BLOCK_SANDSTONE = 52,
	BLOCK_SNOW = 53,
	BLOCK_FIRE = 54,
	BLOCK_LIGHTPINK = 55,
	BLOCK_FORESTGREEN = 56,
	BLOCK_BROWN = 57,
	BLOCK_DEEPBLUE = 58,
	BLOCK_TURQUOISE = 59,
	BLOCK_ICE = 60,
	BLOCK_CERAMICTILE = 61,
	BLOCK_MAGMA = 62,
	BLOCK_PILLAR = 63,
	BLOCK_CRATE = 64,
	BLOCK_STONEBRICK = 65,

	BLOCK_DEFAULT_COUNT
} EBlockIDs;

typedef struct _BulkBlockUpdate {
	World *world; /** Мир, игрокам которого будет отослан пакет */
	cs_bool autosend; /** Автоматически отправлять пакет в случае переполнения буфера блоков */
	struct _BBUData {
		cs_byte count; /** Количество блоков в буфере */
		cs_byte offsets[1024]; /** Смещения блоков в массиве мира */
		BlockID ids[256]; /** Номера блоков */
	} data; /** Буфер блоков */
} BulkBlockUpdate;
#endif
