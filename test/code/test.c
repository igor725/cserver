/*
** Здесь можно наблюдать пример
** структуры плагинов для сервера,
** использовать только для ознакомления
** с PluginAPI.
*/

/*
** Хедеры сервера. Файл core.h должен быть
** всегда подключен в главном файле кода плагина.
*/
#include <core.h>
#include <str.h>
#include <log.h>
#include <client.h>
#include <block.h>
#include <event.h>
#include <command.h>

// Хедеры плагина
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
  if(enabled)
    *((onMessage)param)->type = MT_ANNOUNCE;
}

/*
** При вызове команды из консоли сервера аргумент "caller" будет NULL.
** Также стоит заметить, что "args" тоже будет NULL при отсутствии аргументов.
*/
static cs_bool CHandler_Plugtest(CommandCallData ccdata) {
  String_Copy(ccdata->out, MAX_CMD_OUT, "This command registred by testplugin." DLIB_EXT);
  return true;
}

static cs_bool CHandler_Atoggle(CommandCallData ccdata) {
	// Макрос проверяет была ли запущена команда администратором
	Command_OnlyForOP(ccdata);

  enabled ^= 1;
	String_FormatBuf(ccdata->out, MAX_CMD_OUT, "Announce chat %s", MODE(enabled));
  return true;
}

/*
** Исходя из названия команды,
** даже самому тупенькому будет понятно:
** Эта команда уничтожает сама себя после
** исполнения, то есть, когда она будет
** вызвана однажды - её нельзя будет вызвать
** вновь, вплоть до перезапуска сервера.
*/
static cs_bool CHandler_SelfDestroy(CommandCallData ccdata) {
	Command_UnregisterByName("selfdestroy");
	Command_Print(ccdata, "This command can't be called anymore");
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
static cs_bool CHandler_ClientOnly(CommandCallData ccdata) {
	Command_OnlyForClient(ccdata);

	String_FormatBuf(ccdata->out, MAX_CMD_OUT, "Client-only command called by %s", Client_GetName(ccdata->caller));
	return true;
}

/*
** Дополнительная информация об
** энумах, используемых при объявлении
** структур sBlockDef может быть найдена
** в файле block.h.
*/

static struct _BlockDef myBlock = {
	BLOCK_ID, "My test block", 0,
	{{
		BDSOL_SOLID,
		255,
		8, 14, 80,
		false,
		BDSND_STONE,
		false,
		8,
		BDDRW_OPAQUE,
		0, 0, 0, 0
	}}
};

static struct _BlockDef myExtendedBlock = {
	BLOCK_ID_EXT, "My extended test block", BDF_EXTENDED,
	{{
		BDSOL_SWIM,
		10,
		17, 37, 53, 51, 35, 49,
		false,
		BDSND_GRASS,
		false,
		10, 10, 10,
		13, 13, 13,
		BDDRW_OPAQUE,
		127, 0, 0, 0
	}}
};

static BlockDef myDynBlock = NULL;

cs_int32 Plugin_ApiVer = PLUGIN_API_NUM; // Текущая версия API плагинов.

cs_bool Plugin_Load(void) { // Основная функция, вызывается после подгрузки плагина.
  Event_RegisterVoid(EVT_ONMESSAGE, onmesgfunc); // Регистрация обработчика эвента.
  Command_Register("plugtest", CHandler_Plugtest); // Регистрация обработчика команд.
  Command_Register("atoggle", CHandler_Atoggle);
	Command_Register("selfdestroy", CHandler_SelfDestroy);
	Command_Register("clonly", CHandler_ClientOnly);
	// Любая Log-функция принимает vararg'и и работает также, как и printf.
  Log_Info("Test plugin loaded"); // Отправка в консоль INFO сообщения.
  Log_Debug("It's a debug message");
  Log_Warn("It's a warning message");

	/*
	** С помощью дополнения BlockDefinitions
	** можно регистрировать новые блоки для
	** клиентов. Фунцкия Block_Define принимает
	** указатель на структуру sBlockDef, в
	** которую записана информация о блоке,
	** который необходимо зарегистрировать.
	** Структура sBlockDef содержит 4 поля:
	** флаги, ID блока, его название и
	** параметры. Если нужно создать
	** расширенный блок, то в структуре
	** следует установить флаг BDF_EXTENDED.
	** P.S. Если имеется непреодалимое желание
	** модифицировать имеющийся блок, то после
	** внесения изменений в его параметры
	** необходимо установить флаг BDF_UPDATED
	** (block.flags |= BDF_UPDATED), а затем
	** вызывать функцию Block_UpdateDefinitions.
	*/
	Block_Define(&myBlock);
	Block_Define(&myExtendedBlock);

	/*
	** Структура sBlockDef также может
	** находиться в динамической памяти, для этого
	** её нужно создать через функцию Block_New, где
	** первым аргументами являются ID блока, его имя и
	** флаги соответственно. После завершения выделения
	** памяти функция вернёт BlockDef, поинтер на sBlockDef.
	** При динамической аллокации структуры происходит также
	** КОПИРОВАНИЕ имени блока, соответственно, переданный в
	** функцию const char* не обязан указывать постоянно на
	** имя блока. При динамической аллокации структуре блока
	** автоматически устанавливается флаг BDF_DYNALLOCED, для
	** чего он нужен описано в теле Plugin_Unload.
	*/
	myDynBlock = Block_New(BLOCK_ID_DYN, "My dynamically allocated block", 0);
	Block_Define(myDynBlock);

	/*
	** Эта функция должна вызываться как после изменений
	** в структурах уже зарегистрированных блоков, так и
	** после регистрации новых блоков. Она рассылает
	** всем клиентам пакеты удаляющие или добавляющие
	** новые блоки. Функцию нужно вызывать даже, если
	** на сервере нет игроков, так как она производит
	** манипуляции с полем "flags".
	*/
	Block_UpdateDefinitions();

	/*
	** Если функция вернула true, значит
	** плагин удалось успешно загрузить.
	** Если функция вернёт false, сервер
	** выгрузит динамическую библиотеку
	** плагина из памяти и не будет
	** больше на неё ссылаться.
	*/
  return true;
}

cs_bool Plugin_Unload(void) {
	/*
	** Вызов Unregister функций внутри
	** функции плагина Unload обязателен,
	** так как он говорит серверу, чтобы тот
	** не ссылался больше на эти участки памяти,
	** ибо в скором времени они станут недоступны
	** и обращение к ним приведёт к падению, а нам
	** оно не нужно.
	*/
	Event_Unregister(EVT_ONMESSAGE, (cs_uintptr)onmesgfunc);
	Command_UnregisterByName("plugtest");
	Command_UnregisterByName("atoggle");
	Command_UnregisterByName("selfdestroy");
	Command_UnregisterByName("clonly");

	/*
	** Функция Block_Undefine ТОЛЬКО УСТАНАВЛИВАЕТ
	** ФЛАГИ, она не производит больше никаких
	** манипуляций. Тем не менее, блоки, имеющие
	** флаг BDF_UNDEFINED не будут отправлены игроку
	** даже если после Block_Undefine не была вызвана
	** функция Block_UpdateDefinitions.
	*/
	Block_Undefine(BLOCK_ID);
	Block_Undefine(BLOCK_ID_EXT);
	Block_Undefine(BLOCK_ID_DYN);

	/*
	** Здесь вызов Block_UpdateDefinitions нужен, чтобы
	** разослать игрокам пакет RemoveBlockDefinition,
	** убрать блок из массива, а также чтобы высвободить
	** место, выделенное под поле "name" и саму структуру,
	** если в ней установлен флаг BDF_DYNALLOCED.
	** Этот вызов внутри функции Unload играет важную роль,
	** сравнимую с Command_Unregister и Event_Unregister.
	*/
	Block_UpdateDefinitions();

	/*
	** Возврат true говорит о том, что
	** плагин может быть выгружен в данный
	** момент без ущерба работоспособности
	** сервера. Если функция вернёт false
	** то плагин останется в памяти
	** нетронутым.
	*/
	return true;
}
