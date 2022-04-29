#ifndef COMMAND_H
#define COMMAND_H
#include "core.h"
#include "client.h"
#include "types/list.h"
#include "types/command.h"

#define COMMAND_SETUSAGE(str) cs_str cmdUsage = str;
#define COMMAND_PRINT(str) return String_Copy(ccdata->out, MAX_CMD_OUT, str) > 0
#define COMMAND_PRINTF(f, ...) return String_FormatBuf(ccdata->out, MAX_CMD_OUT, f, ##__VA_ARGS__) > 0
#define COMMAND_APPEND(str) String_Append(ccdata->out, MAX_CMD_OUT, str)
#define COMMAND_GETARG(a, s, n) String_GetArgument(ccdata->args, a, s, n)

#define COMMAND_APPENDF(buf, sz, fmt, ...) (\
String_FormatBuf(buf, sz, fmt, ##__VA_ARGS__), \
String_Append(ccdata->out, MAX_CMD_OUT, buf))

#define COMMAND_PRINTLINE(line) \
if(ccdata->caller) \
	Client_Chat(ccdata->caller, MESSAGE_TYPE_CHAT, line); \
else \
	Log_Info(line);

#define COMMAND_PRINTFLINE(f, ...) \
if(ccdata->caller) { \
	String_FormatBuf(ccdata->out, MAX_CMD_OUT, f, ##__VA_ARGS__); \
	Client_Chat(ccdata->caller, MESSAGE_TYPE_CHAT, ccdata->out); \
} else \
	Log_Info(f, ##__VA_ARGS__);

#define COMMAND_TESTOP() \
if(ccdata->caller && !Client_IsOP(ccdata->caller)) { \
	COMMAND_PRINT(Sstor_Get("CMD_NOPERM")); \
}

#define COMMAND_PRINTUSAGE \
COMMAND_PRINTF("Usage: %s", cmdUsage)

#define COMMAND_FUNC(N) \
static cs_bool svcmd_##N(CommandCallData *ccdata)

#define COMMAND_ADD(N, F, H) \
Command_Register(#N, H, (cmdFunc)svcmd_##N, F)

#define COMMAND_REMOVE(N) \
Command_UnregisterByFunc((cmdFunc)svcmd_##N)

#define Command_DeclareBunch(N) static CommandRegBunch N[] =
#define COMMAND_BUNCH_ADD(N, F, H) {#N, H, (cmdFunc)svcmd_##N, F},
#define COMMAND_BUNCH_END {NULL, NULL, NULL, 0x00}

void Command_RegisterDefault(void);
void Command_UnregisterAll(void);

/**
 * @brief Обрабатывает переданную строку как команду.
 * 
 * @param str команда с аргументами (массив может измениться в процессе выполнения)
 * @param caller игрок, вызвавший команду
 * @return true - команда выполнена успешно, false - произошла какая-то ошибка
 */
API cs_bool Command_Handle(cs_char *str, Client *caller);

/**
 * @brief Регистрирует новую команду.
 * 
 * @param name название команды
 * @param descr описание команды
 * @param func функция команды
 * @param flags флаги команды
 * @return указатель на структуру команды
 */
API Command *Command_Register(cs_str name, cs_str descr, cmdFunc func, cs_byte flags);

/**
 * @brief Удаляет зарегистрированную ранее команду.
 * 
 * @param cmd указатель на структуру команды
 */
API void Command_Unregister(Command *cmd);

API cs_bool Command_RegisterBunch(CommandRegBunch *bunch);

API void Command_UnregisterBunch(CommandRegBunch *bunch);

/**
 * @brief Возвращает указатель на структуру команды по её имени.
 * 
 * @param name имя команды
 * @return указатель на структуру команды
 */
API Command *Command_GetByName(cs_str name);

/**
 * @brief Удаляет зарегистрированную ранее команду.
 * 
 * @param func функция команды
 */
API void Command_UnregisterByFunc(cmdFunc func);

/**
 * @brief Возвращает имя команды.
 * 
 * @param cmd указатель на структуру команды
 * @return имя команды
 */
API cs_str Command_GetName(Command *cmd);

/**
 * @brief Устанавливает короткое имя для команды.
 * Короткое имя не может быть длиннее 6 символов.
 * 
 * @param cmd указатель на структуру команды
 */
API cs_bool Command_SetAlias(Command *cmd, cs_str alias);

/**
 * @brief Присваивает команде указатель для различных целей.
 * 
 * @param cmd указатель на структуру команды
 * @param ud указатель на данные
 * @return API 
 */
API void Command_SetUserData(Command *cmd, void *ud);

/**
 * @brief Возвращает ранее установленный команде указатель.
 * 
 * @param cmd указатель на структуру команды
 * @return указатель на данные
 */
API void *Command_GetUserData(Command *cmd);

extern AListField *Command_Head; /** Список зарегистрированных команд */
#endif // COMMAND_H
