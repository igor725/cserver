#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include "core.h"
#include "types/websock.h"

/**
 * @brief Пытается выполнить стандартное рукопожатие
 * вебсокета с указанным в структуре сокетом.
 * 
 * @param ws указатель на структуру вебсокета
 * @return true - рукопожатие завершилось успешно,
 * false - клиент что-то сделал не так (или сервер???)
 */
API cs_bool WebSock_DoHandshake(WebSock *ws);

/**
 * @brief Читает из сокета фрейм вебсокета.
 * Внимание: Может вернуть false даже если сокет в порядке!
 * В случае ошибки всегда проверяйте WebSock_GetErrorCode(),
 * если там значение WS_ERROR_CONTINUE, то это значит, что
 * нужен ещё один вызов, чтобы получить фрейм полностью.
 * 
 * @param ws указатель на структуру вебсокета
 * @return true - фрейм полностью прочитан,
 * false - что-то не так
 */
API cs_bool WebSock_ReceiveFrame(WebSock *ws);

/**
 * @brief Отправляет фрейм клиенту.
 * 
 * @param ws указатель на структуру вебсокета
 * @param opcode опкод фрейма
 * @param buf данные, которые будут записаны в фрейм
 * @param len длинна данных
 * @return true - фрейм отправлен успешно,
 * false - похоже, что соединение разорвано.
 */
API cs_bool WebSock_SendFrame(WebSock *ws, cs_byte opcode, const cs_char *buf, cs_uint16 len);

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
