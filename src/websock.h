#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include "core.h"
#include "types/websock.h"

/**
 * @brief Эдакий комбайн, выполняет хендшейк и чтение из
 * сокета фреймов. Рассчитан на работу с нонблок сокетами.
 * Внимание: Может вернуть false даже если сокет в порядке!
 * В случае ошибки всегда проверяйте WebSock_GetErrorCode(),
 * если там значение WEBSOCK_ERROR_CONTINUE, то это значит,
 * что нужен ещё один вызов, чтобы получить фрейм полностью.
 * 
 * @param ws указатель на структуру вебсокета
 * @param sock сокет, из которого читаем данные
 * @return true - фрейм полностью прочитан,
 * false - что-то не так
 */
API cs_bool WebSock_Tick(WebSock *ws, Socket sock);

/**
 * @brief Отправляет фрейм клиенту.
 * 
 * @param sock сокет, в который нужно послать фрейм
 * @param opcode опкод фрейма
 * @param buf данные, которые будут записаны в фрейм
 * @param len длинна данных
 * @return true - фрейм отправлен успешно,
 * false - похоже, что соединение разорвано.
 */
API cs_bool WebSock_SendFrame(Socket sock, cs_byte opcode, const cs_char *buf, cs_uint16 len);

/**
 * @brief Возвращает код последней произошедшей ошибки.
 * 
 * @param ws указатель на структуру вебсокета
 * @return код ошибки из перечисления EWebSockErrors
 */
API EWebSockErrors WebSock_GetErrorCode(WebSock *ws);

/**
 * @brief Делает тоже самое, что и WebSock_GetErrorCode,
 * но она уже возвращает ошибку в качестве строки.
 * 
 * @param ws указатель на структуру вебсокета
 * @return строка с ошибкой
 */
API cs_str WebSock_GetError(WebSock *ws);
#endif // WEBSOCKET_H
