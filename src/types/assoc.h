#ifndef ASSOCTYPES_H
#define ASSOCTYPES_H
#include "core.h"

/**
 * @brief Перечисление возможных объектов для создания ассоцииации
 * 
 */
typedef enum _EAssocBindType {
	ASSOC_BIND_WORLD, /** Тип используется для ассоциации с мирами */
	ASSOC_BIND_CLIENT /** Тип используется для ассоциации с игроками */
} EAssocBindType;

/**
 * @brief Уникальный номер ассоциативного типа
 * 
 */
typedef cs_int16 AssocType;

/**
 * @brief Ассоциированная память
 * 
 */
typedef void *AssocMem;
#endif
