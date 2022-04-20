#ifndef HEARTBEAT_H
#define HEARTBEAT_H
#include "core.h"
#include "types/client.h"
#include "types/platform.h"

typedef cs_bool(*heartbeatKeyChecker)(cs_str secret, Client *client);

typedef struct _Heartbeat {
	heartbeatKeyChecker checker;
	cs_str domain, playurl, reqpath;
	Waitable *isdone;
	cs_bool started,
	ispublic, issecure,
	isannounced;
	cs_uint16 delay;
} Heartbeat;

void Heartbeat_StopAll(void);

/**
 * @brief Создаёт новый heartbeat.
 * 
 * @return указатель на heartbeat
 */
API Heartbeat *Heartbeat_New(void);

/**
 * @brief Устанавливает домен для heartbeat.
 * 
 * @param self указатель на heartbeat
 * @param domain целевой сервер для отправки hearbeat запросов
 * @return true - домен установлен, false - heartbeat уже запущен
 */
API cs_bool Heartbeat_SetDomain(Heartbeat *self, cs_str domain);

/**
 * @brief Устанавливает путь запроса.
 * Стоковый heartbeat игры имеет следующий путь:
 * /heartbeat.jsp
 * 
 * @param self указатель на heartbeat
 * @param path путь запроса
 * @return true - путь установлен, false - heartbeat уже запущен
 */
API cs_bool Heartbeat_SetRequestPath(Heartbeat *self, cs_str path);

/**
 * @brief Устанавливает строку, которая будет использована
 * при поиске игровой ссылки, в ответе от heartbeat сервера.
 * 
 * @param self указатель на heartbeat
 * @param url кусок ссылки, который нужно найти в ответе от сервера
 * @return true - ссылка установлена, false - heartbeat уже запущен
 */
API cs_bool Heartbeat_SetPlayURL(Heartbeat *self, cs_str url);

/**
 * @brief Устанавливает функцию для проверки ключа игрока.
 * Если функция не была задана до Heartbeat_Run, то будет
 * использована стандартная функция проверки ключа игрока.
 * 
 * @param self указатель на heartbeat
 * @param func функция проверки ключа
 * @return true - функция установлена, false - heartbeat уже запущен
 */
API cs_bool Heartbeat_SetKeyChecker(Heartbeat *self, heartbeatKeyChecker func);

/**
 * @brief Устанавливает видимость сервера в списке серверов.
 * 
 * @param self указатель на hearbeat
 * @param state значение видимости
 */
API void Heartbeat_SetPublic(Heartbeat *self, cs_bool state);

/**
 * @brief Задержка между запросами к heartbeat серверу.
 * 
 * @param self указатель на heartbeat
 * @param delay задержка (мсек)
 */
API void Heartbeat_SetDelay(Heartbeat *self, cs_uint16 delay);

/**
 * @brief Запускает поток, осуществляющий запросы к
 * heartbeat серверу.
 * 
 * @param self указатель на heartbeat
 * @return true - запуск успешен, false - какой-то из параметров не был
 */
API cs_bool Heartbeat_Run(Heartbeat *self);

/**
 * @brief Останавливает запросы к heartbeat серверу,
 * которые осуществлялись с помощью данного контекста.
 * 
 * @param указатель на heartbeat
 */
API void Heartbeat_Close(Heartbeat *self);

/**
 * @brief Производит проверку ключа игрока на действительность
 * всеми доступными функциями проверки ключа. Для игроков с
 * 127.0.0.1 всегда возвращает true.
 * 
 * @param client 
 * @return cs_bool 
 */
cs_bool Heartbeat_Validate(Client *client);
#endif // HEARTBEAT_H
