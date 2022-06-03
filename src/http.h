#ifndef HTTP_H
#define HTTP_H
#include "core.h"

#define HTTP_USERAGENT SOFTWARE_FULLNAME

#if defined(HTTP_USE_WININET_BACKEND)
// Start of <wininet.h>
#	define INTERNET_SERVICE_HTTP 3
#	define INTERNET_OPEN_TYPE_PRECONFIG 0
#	define INTERNET_FLAG_SECURE 0x00800000
	typedef void *HINTERNET;
	typedef cs_uint16 INTERNET_PORT;
// End of <wininet.h>

	typedef struct _Http {
		cs_bool secure;
		HINTERNET conn, req;
	} Http;
#elif defined(HTTP_USE_CURL_BACKEND)
#	define CURLOPT_WRITEDATA 10000 + 1
#	define CURLOPT_URL 10000 + 2
#	define CURLOPT_USERAGENT 10000 + 18
#	define CURLOPT_FOLLOWLOCATION 0 + 52
#	define CURLOPT_WRITEFUNCTION 20000 + 11
#	define CURLE_OK 0

	typedef void CURL;
	typedef struct _Http {
		cs_bool secure;
		cs_str domain;
		cs_size buflen, rsplen;
		cs_char *path, *buf;
		CURL *handle;
	} Http;
#else
#	error No HTTP backend selected
#endif

#ifndef CORE_BUILD_PLUGIN
	void Http_Uninit(void);
#endif

/**
 * @brief Устанавливает соединение с указанным сервером
 * по HTTP протоколу.
 * 
 * @param http контекст http
 * @param domain домен сервера
 * @return true - подключение успешно, false - произошла внутренняя ошибка
 */
API cs_bool Http_Open(Http *http, cs_str domain);

/**
 * @brief Выполняет запрос к серверу с указанным URL.
 * 
 * @param http контекст http
 * @param url URL запроса
 * @return true - запрос успешен, false - ошибка при выполнении запроса
 */
API cs_bool Http_Request(Http *http, cs_str url);

/**
 * @brief Читает ответ сервера в указанный буфер.
 * На данный момент, если буфера не хватает для чтения
 * ответа от сервера, то данные, на которые не хватило
 * места ПОТЕРЯЮТСЯ.
 * 
 * @param http контекст http
 * @param buf буфер для ответа
 * @param sz размер буфера для ответа
 * @return количество прочтённых байт
 */
API cs_ulong Http_ReadResponse(Http *http, cs_char *buf, cs_ulong sz);

/**
 * @brief Возвращает контекст в неинициализированное состояние,
 * в котором он находился до вызова Http_Open.
 * 
 * @param http контекст http
 * @return API 
 */
API void Http_Cleanup(Http *http);
#endif // HTTP_H
