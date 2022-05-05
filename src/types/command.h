#ifndef COMMANDTYPE_H
#define COMMANDTYPE_H
#include "core.h"
#include "str.h"
#include "log.h"
#include "strstor.h"
#include "client.h"

#define CMDF_NONE      0x00
#define CMDF_OP        BIT(0)
#define CMDF_CLIENT    BIT(1)
#define CMDF_RESERVED0 BIT(2)
#define CMDF_RESERVED1 BIT(3)
#define CMDF_RESERVED2 BIT(4)

/**
 * @brief Структура с информацией о вызове команды.
 * 
 */
typedef struct _CommandCallData {
	struct _Command *const command; /** Вызванная команда */
	cs_str const args; /** Аргументы с которыми команда была вызвана */
	Client *const caller; /** Игрок, вызвавший команду, NULL в случае вызова из консоли*/
	cs_char *const out; /* Массив, в который команда будет записывать вывод*/
} CommandCallData;

/**
 * @brief Функция команды.
 * 
 */
typedef cs_bool(*cmdFunc)(CommandCallData *);

typedef struct _CommandRegBunch {
	cs_str name, descr;
	cmdFunc func;
	cs_byte flags;
} CommandRegBunch;

typedef struct _Command {
	cs_str name;
	cs_char alias[7];
	cs_byte flags;
	cmdFunc func;
	cs_str descr;
	void *data;
} Command;
#endif
