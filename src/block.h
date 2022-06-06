#ifndef BLOCK_H
#define BLOCK_H
#include "core.h"
#include "types/world.h"
#include "types/cpe.h"
#include "types/block.h"

BlockDef *Block_GetDefinition(World *world, BlockID id);

/**
 * @brief Возвращает фоллбек блок для указанного блока
 * на случай, если клиент не поддерживает дополнения
 * CPE.
 * 
 * @param world целевой мир
 * @param id любой уникальный номер блока
 * @return уникальный номер vanilla-блока
 */
API BlockID Block_GetFallbackFor(World *world, BlockID id);

/**
 * @brief Проверяет, существует ли блок под указанным номером.
 * 
 * @param world целевой мир
 * @param id уникальный номер блока
 * @return true - блок существует, false - блок не существует
 */
API cs_bool Block_IsValid(World *world, BlockID id);

/**
 * @brief Возвращает имя указанного блока. В случае, если блок
 * не существует вернёт строку "Unknown block".
 * 
 * @param world целевой мир
 * @param id уникальный номер блока
 * @return строка с именем блока
 */
API cs_str Block_GetName(World *world, BlockID id);

API cs_bool Block_IsDefinedFor(World *world, BlockDef *bdef);

/**
 * @brief Очищает у блока флаги UPDATED и UNDEFINED и
 * добавляет его к массиву указанного мира.
 * 
 * @param world целевой мир
 * @param bdef структура, описывающая блока
 * @return true - регистрация прошла успешно, false - блок с таким id уже зарегистрирован
 */
API cs_bool Block_Define(World *world, BlockID id, BlockDef *bdef);

/**
 * @brief Удаляет указанный блок для одного мира
 * 
 * @param world целевой мир
 * @param bdef структура, описывающая блок
 * @return true - блок удалён, false - блок не был зарегистрирован
 */
API cs_bool Block_Undefine(World *world, BlockDef *bdef);

/**
 * @brief Устанавливает указанному блоку флаг UNDEFINED
 * 
 * @param bdef структура, описывающая блок
 */
API void Block_UndefineGlobal(BlockDef *bdef);

/**
 * @brief Рассылает всем клиентам миров, для которых
 * данным блок зарегистрирован обновления состояния
 * блока.
 * 
 * @param bdef структура, описывающая блок
 */
API void Block_UpdateDefinition(BlockDef *bdef);

/**
 * @brief Добавляет блок в кучу.
 * 
 * @param bbu указатель на кучу
 * @param offset смещение блока в мире
 * @param id уникальный номер блока
 * @return true - блок добавлен, false - произошла ошибка
 */
API cs_bool Block_BulkUpdateAdd(BulkBlockUpdate *bbu, cs_uint32 offset, BlockID id);

/**
 * @brief Рассылает всем игрокам указанного в куче мира обновлённые блоки.
 * 
 * @param bbu указатель на кучу
 * @return true - пакет добавлен в очередь, false - отправка провалилась
 */
API cs_bool Block_BulkUpdateSend(BulkBlockUpdate *bbu);

/**
 * @brief Очищает кучу от добавленных в неё блоков.
 * 
 * @param bbu указатель на кучу
 */
API void Block_BulkUpdateClean(BulkBlockUpdate *bbu);
#endif // BLOCK_H
