/*
** Здесь можно наблюдать пример
** структуры плагинов для сервера,
** использовать только для ознакомления
** с PluginAPI.
*/

// Хедеры сервера
#include <core.h>
#include <event.h>
#include <command.h>

/*
  Хедеры плагина, их желательно
  подключать после хедеров сервера
*/
#include "test.h"

/*
** В функцию эвента передаётся первый параметр,
** который был передан функции вызова эвента,
** если параметр был один, то передаётся только он,
** без поинтера на пачало параметров функции.
** Мне показалось удобным кастование аргументов
** в структуру для дальнейшего взаимодействия с ними
** более удобного способа я не придумал.
*/
static void onmesgfunc(void* param) {
  if(enabled) {
    onMessage_t* a = (onMessage_t*)param;
    *a->type = CPE_ANNOUNCE;
  }
}

/*
** При вызове команды из консоли сервера аргумент "caller" будет NULL.
** Также стоит заметить, что "args" тоже будет NULL при отсутствии аргументов.
*/
static bool CHandler_Plugtest(const char* args, CLIENT* caller, char* out) {
  String_Copy(out, CMD_MAX_OUT, "This command registred by testplugin." DLIB_EXT);
  return true;
}

static bool CHandler_Atoggle(const char* args, CLIENT* caller, char* out) {
	// Макрос проверяет была ли запущена команда администратором
	Command_OnlyForOP;

  enabled = !enabled;
	String_Copy(out, CMD_MAX_OUT, "Announce chat: ");
  String_Append(out, CMD_MAX_OUT, enabled ? "enabled" : "disabled");
  return true;
}

/*
** Если в начало обработчика команды
** сунуть макрос Command_OnlyForClient,
** то команда будет выполнена только в том
** случае, если вызвана она была игроком,
** в противном случае в консоль выводится
** сообщение о том, что команду может вызвать
** только игрок.
*/
static bool CHandler_ClientOnly(const char* args, CLIENT* caller, char* out) {
	Command_OnlyForClient;

	String_FormatBuf(out, CMD_MAX_OUT, "Client-only command called by %s", caller->playerData->name);
	return true;
}

EXP int Plugin_ApiVer = 100; // Текущая версия API плагинов 1.0.0.
EXP bool Plugin_Init() { // Основная функция, вызывается после подгрузки плагина.
  Event_RegisterVoid(EVT_ONMESSAGE, onmesgfunc); // Регистрация обработчика эвента.
  Command_Register("plugtest", CHandler_Plugtest);
  Command_Register("atoggle", CHandler_Atoggle);
	Command_Register("clonly", CHandler_ClientOnly);
  Log_Info("Test plugin loaded"); // Отправка в консоль INFO сообщения.
  Log_Debug("It's a debug message");
  Log_Warn("It's a warning message");
  return true;
}
