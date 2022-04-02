/**
 * @file plugin.h
 * @author igor725 (gvaldovigor@gmail.com)
 * @brief Этот файл должен подключаться каждым плагином сервера.
 * Он реализует основные макросы и экспортирует функции плагина.
 * Примеры плагинов для сервера можно найти здесь:
 * https://github.com/igor725/cs-lua
 * https://github.com/igor725/cs-base
 * https://github.com/igor725/cs-survival
 * https://github.com/igor725/cs-worldedit
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef PLUGIN_H
#define PLUGIN_H
#include "core.h"
#include "platform.h"
#include "types/plugin.h"

#ifndef CORE_BUILD_PLUGIN
#	define Plugin_Lock(_p) Mutex_Lock((_p)->mutex)
#	define Plugin_Unlock(_p) Mutex_Unlock((_p)->mutex)

	void Plugin_LoadAll(void);
	void Plugin_UnloadAll(cs_bool force);
#else
	/**
	 * @brief Выполняется сервером в момент загрузки плагина.
	 * Если функция вернёт false, плагин будет немедленно
	 * выгружен из памяти.
	 * 
	 * @return true - плагин успешно загрузился, false - произошёл какой-то прикол
	 */
	EXP cs_bool Plugin_Load(void);

	/**
	 * @brief Выполняется сервером в момент выгрузки плагина.
	 * Например, при вызове серверной команды /plugin unload <name>
	 * игроком, либо консолью.
	 * 
	 * @param force true - возвращённое значение будет проигнорировано
	 * и плагин выгрузится в любом случае, false - плагин не будет
	 * выгружен, если эта функция вернёт false.
	 * @return true - плагин может завершить свою работу сейчас,
	 * false - плагин не может быть выгружен.
	 */
	EXP cs_bool Plugin_Unload(cs_bool force);

	/**
	 * @brief Вызывается сервером, когда тот найдёт запрошенный
	 * плагином интерфейс через Plugin_RequestInterface.
	 * Если в данный момент никакой из плагинов не предоставляет
	 * указанный интерфейс, сервер добавит запрос в холд-лист,
	 * далее плагин должен терпеливо ждать вызова этой функции.
	 * 
	 * @param name название интерфейса
	 * @param ptr указатель на структуру интерфейса (для каждого плагина она индивидуальна)
	 * @param size размер структуры интерфейса
	 */
	EXP void Plugin_RecvInterface(cs_str name, void *ptr, cs_size size);
	EXP cs_int32 Plugin_ApiVer, Plugin_Version;

	// Небольшой макрос для облегчения жизни при объявлении интерфейсов плагина
#	define Plugin_DeclareInterfaces EXP PluginInterface Plugin_Interfaces[]; \
	PluginInterface Plugin_Interfaces[] = 

	/**
	 * @brief Макрос, который должен выполнить каждый плагин. Он устанавливает версию API,
	 * которую использует данный плагин в целях проверки совместимости с функциями,
	 * предоставляемыми сервером. Также он устанавливает текущую версию плагина, которая
	 * показывается при вызове команды /plugin list.
	 * 
	 * P.S. На данном этапе развития сервера версия API не меняется,
	 * она заморожена на значении 001, так как итоговый набор функций
	 * ещё не сформирован окончательно.
	 */
#	define Plugin_SetVersion(ver) cs_int32 Plugin_ApiVer = PLUGIN_API_NUM, Plugin_Version = ver

	// Макросы для создания массива интерфейсов
#	define PLUGIN_IFACE_END {NULL, NULL, 0}
#	define PLUGIN_IFACE_ADD(n, i) {n, &(i), sizeof(i)},
#endif

/**
 * @brief Загружает указанный плагин.
 * 
 * @param name название плагина
 * @return true - плагин загружен успешно, false - произошли
 * технические шоколадки.
 */
API cs_bool Plugin_LoadDll(cs_str name);

/**
 * @brief 
 * 
 * @param plugin название плагина
 * @param force будет ли проигнорировано значение, которое вернёт функция Plugin_Unload
 * (может привести к потере данных!)
 * @return true - плагин был выгружен успешно, false - что-то не так
 */
API cs_bool Plugin_UnloadDll(Plugin *plugin, cs_bool force);

/**
 * @brief Возвращает указатель на структуру
 * плагина, используется функцией Plugin_UnloadDll.
 * 
 * Настоятельно рекомендуется не изменять вручную поля
 * структуры. Это может привести к неопределённому поведению.
 * 
 * @param name название плагина
 * @return указатель на структуру плагина
 */
API Plugin *Plugin_Get(cs_str name);

/**
 * @brief Запрашивает у сервера указанный интерфейс.
 * 
 * Если эта функция вернула true, это ещё не гарантирует то,
 * что плагин получит указанный интерфейс. Он придёт только тогда
 * когда в сервер загрузится плагин, реализующий его. До этого
 * момента запрос будет находиться в холд-листе. Если указанный
 * интерфейс будет найден в момент выполнения этой функции, то
 * ей же будет вызван коллбек Plugin_RecvInterface.
 * 
 * Указатель на Plugin_RecvInterface используется в идентификационных
 * целях, чтобы сервер знал, какому плагину нужно отправить интерфейс.
 * 
 * Названия интерфейсов чувствительны к регистру!!
 * 
 * @param irecv указатель на функцию Plugin_RecvInterface
 * @param iname название интерфейса
 * @return true - запрос успешен, false - этот интерфейс уже был запрошен
 */
API cs_bool Plugin_RequestInterface(pluginReceiveIface irecv, cs_str iname);

/**
 * @brief Отключает указанный интерфейс от плагина.
 * 
 * @param irecv указатель на функцию Plugin_RecvInterface
 * @param iname название интерфейса
 * @return true - интерфейс отключен, false - такой интерфейс не был запрошел
 */
API cs_bool Plugin_DiscardInterface(pluginReceiveIface irecv, cs_str iname);

// Массив всех загруженных плагинов
VAR Plugin *Plugins_List[MAX_PLUGINS];
#endif // PLUGIN_H
