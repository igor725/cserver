#ifndef COMPR_H
#define COMPR_H
#include "core.h"
#include "types/compr.h"

/**
 * @brief Инициализирует архиватор.
 * 
 * @param ctx указатель на контекст архиватора
 * @param type тип архиватора
 * @return true - архиватор инициализорван, false - ошибка инициализации
 */
API cs_bool Compr_Init(Compr *ctx, ComprType type);

/**
 * @brief Возвращает CRC32 хеш для переданных данных.
 * 
 * @param data указатель на данные
 * @param len размер данных в байтах
 * @return CRC32 хеш
 */
API cs_ulong Compr_CRC32(const cs_byte *data, cs_uint32 len);

/**
 * @brief Устанавливает архиватору входной буфер.
 * 
 * @param ctx указатель на контекст архиватора
 * @param data указатель на буфер
 * @param size размер буфера
 */
API void Compr_SetInBuffer(Compr *ctx, void *data, cs_uint32 size);

/**
 * @brief Устанавливает архиватору выходной буфер.
 * 
 * @param ctx указатель на контекст архиватора
 * @param data указатель на буфер
 * @param size размер буфера
 */
API void Compr_SetOutBuffer(Compr *ctx, void *data, cs_uint32 size);

/**
 * @brief Выполняет один шаг архиватора
 * 
 * @param ctx указатель на контекст архиватора
 * @return true - архиватор завершил работу, false - требуется ещё один шаг
 */
API cs_bool Compr_Update(Compr *ctx);

/**
 * @brief Возвращает текст ошибки по её коду.
 * 
 * @param code код ошибки
 * @return строка с ошибкой
 */
API cs_str Compr_GetError(cs_int32 code);

/**
 * @brief Возвращает последнюю произошедшую в архиваторе ошибку.
 * 
 * @param ctx указатель на контекст архиватора
 * @return строка с ошибкой
 */
API cs_str Compr_GetLastError(Compr *ctx);

/**
 * @brief Возвращает архиватор в начальное состояние.
 * 
 * @param ctx указатель на контекст архиватора
 */
API void Compr_Reset(Compr *ctx);

/**
 * @brief Высвобождает память, выделенную под внутренний zlib контекст архиватора.
 * 
 * @param ctx указатель на контекст архиватора
 */
API void Compr_Cleanup(Compr *ctx);

/**
 * @brief Отключает библиотеку zlib.
 * (При следующем вызове Compr_Init произойдёт повторное подключение)
 * 
 */
void Compr_Uninit(void);
#endif
