#ifndef GENERATORS_H
#define GENERATORS_H
#include "core.h"
#include "types/list.h"
#include "types/world.h"

#define GENERATOR_SEED_FROM_TIME (cs_int32)-1

typedef cs_bool(*GeneratorRoutine)(World *world, cs_int32 seed);

cs_bool Generators_Init(void);
void Generators_UnregisterAll(void);

/**
 * @brief Добавляет новый генератор миров.
 * 
 * @param name название генератора
 * @param gr функция генератора
 * @return true - генератор зарегистрирован, false - генератор с таким именем уже существует
 */
API cs_bool Generators_Add(cs_str name, GeneratorRoutine gr);

/**
 * @brief Удаляет добавленный ранее генератор миров.
 * 
 * @param name название генератора
 * @return true - генератор удалён, false - генератор с таким именем не был зарегистрирован 
 */
API cs_bool Generators_Remove(cs_str name);

/**
 * @brief Удаляет добавленный ранее генератор миров.
 * 
 * @param gr функция генератора
 * @return true - генератор удалён, false - генератор с такой функцией не был зарегистрирован
 */
API cs_bool Generators_RemoveByFunc(GeneratorRoutine gr);

/**
 * @brief Применяет к указанному миру генератор.
 * 
 * @param world указатель на мир
 * @param name имя генератора
 * @param data пока что не используется
 * @return true - генератор выполнил работу успешно, false - что-то пошло не так
 */
API cs_bool Generators_Use(World *world, cs_str name, cs_int32 seed);

/**
 * @brief Возвращает функцию генератора по его имени.
 * 
 * @param name имя генератора
 * @return функция генератора
 */
API GeneratorRoutine Generators_Get(cs_str name);
#endif // GENERATORS_H
