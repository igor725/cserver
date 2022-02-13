#ifndef BLOCK_H
#define BLOCK_H
#include "core.h"
#include "world.h"

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
	BLOCK_OBSIDIAN = 49 /** Обсидиан */
} EBlockIDs;

/**
 * @name BDF_*
 * 
 * Флаги для динамических блоков.
 * 
 */
/* @{ */
#define BDF_EXTENDED BIT(0) /** Блок использует расширенную спецификацию */
#define BDF_DYNALLOCED BIT(1) /** Блок был создан в динамической памяти */
#define BDF_UPDATED BIT(2) /** Блок был разослан клиентам */
#define BDF_UNDEFINED BIT(3) /** Блок удалён */
/* @} */

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

/**
 * @brief Структура, описывающая создаваемый блок.
 * 
 */
typedef struct _BlockDef {
	cs_char name[65]; /** Название блока */
	BlockID id; /** Уникальный номер блока */
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
			cs_byte transmitsLight; /** Пропускает ли блок свет */
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

typedef struct _BulkBlockUpdate {
	World *world; /** Мир, игрокам которого будет отослан пакет */
	cs_bool autosend; /** Автоматически отправлять пакет в случае переполнения буфера блоков */
	struct _BBUData {
		cs_byte count; /** Количество блоков в буфере */
		cs_byte offsets[1024]; /** Смещения блоков в массиве мира */
		BlockID ids[256]; /** Номера блоков */
	} data; /** Буфер блоков */
} BulkBlockUpdate;

/**
 * @brief Проверяет, существует ли блок под указанным номером.
 * 
 * @param id уникальный номер блока
 * @return true - блок существует, false - блок не существует
 */
API cs_bool Block_IsValid(BlockID id);

/**
 * @brief Возвращает имя указанного блока. В случае, если блок
 * не существует вернёт строку "Unknown block".
 * 
 * @param id уникальный номер блока
 * @return строка с именем блока
 */
API cs_str Block_GetName(BlockID id);

/**
 * @brief Создаёт новый блок в динамической памяти.
 * 
 * @param id уникальный номер блока
 * @param name название блока
 * @param flags флаги блока
 * @return структура, описывающая блок
 */
API BlockDef *Block_New(BlockID id, cs_str name, cs_byte flags);

/**
 * @brief Высвобождает память, выделенную под динамический блок.
 * Данную функцию следует вызывать после функций Block_Undefine
 * и Block_UpdateDefinitions если блок уже был отправлен игрокам,
 * иначе они не будут знать о том, что этот блок более не существует.
 * 
 * @param bdef структура, описывающая блок
 */
API void Block_Free(BlockDef *bdef);

/**
 * @brief Очищает у блока флаги UPDATED и UNDEFINED
 * 
 * @param bdef структура, описывающая блока
 * @return true - регистрация прошла успешно, false - блок с таким id уже зарегистрирован
 */
API cs_bool Block_Define(BlockDef *bdef);

/**
 * @brief Возвращает структуру блока по его номеру.
 * 
 * @param id уникальный номер блока
 * @return структура, описывающая, блок
 */
API BlockDef *Block_GetDefinition(BlockID id);

/**
 * @brief Очищает у указанного блока 
 * 
 * @param bdef структура, описывающая блок
 * @return true - флаги блока обновлены, false - блок не был зарегистрирован
 */
API cs_bool Block_Undefine(BlockDef *bdef);

/**
 * @brief Рассылает всем клиентам пакеты регистрации блоков.
 * 
 */
API void Block_UpdateDefinitions(void);

/**
 * @brief Добавляет блок в кучу.
 * 
 * @param bbu указатель на кучу
 * @param offset смещение блока в мире
 * @param id уникальный номер блока
 * @return API 
 */
API cs_bool Block_BulkUpdateAdd(BulkBlockUpdate *bbu, cs_uint32 offset, BlockID id);

/**
 * @brief Рассылает всем игрокам указанного в куче мира обновлённые блоки.
 * 
 * @param bbu указатель на кучу
 */
API void Block_BulkUpdateSend(BulkBlockUpdate *bbu);

/**
 * @brief Очищает кучу от добавленных в неё блоков.
 * 
 * @param bbu указатель на кучу
 */
API void Block_BulkUpdateClean(BulkBlockUpdate *bbu);
#endif // BLOCK_H
