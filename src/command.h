#ifndef COMMAND_H
#define COMMAND_H
#include "core.h"
#include "str.h"
#include "client.h"

#define COMMAND_SETUSAGE(str) \
cs_str cmdUsage = str;

#define COMMAND_PRINT(str) \
String_Copy(ccdata->out, MAX_CMD_OUT, str); \
return true;

#define COMMAND_PRINTF(f, ...) \
String_FormatBuf(ccdata->out, MAX_CMD_OUT, f, ##__VA_ARGS__); \
return true;

#define COMMAND_APPEND(str) \
String_Append(ccdata->out, MAX_CMD_OUT, str);

#define COMMAND_APPENDF(buf, sz, fmt, ...) \
String_FormatBuf(buf, sz, fmt, ##__VA_ARGS__); \
String_Append(ccdata->out, MAX_CMD_OUT, buf);

#define COMMAND_GETARG(a, s, n) \
String_GetArgument(ccdata->args, a, s, n)

#define COMMAND_PRINTUSAGE \
COMMAND_PRINTF("Usage: %s.", cmdUsage);

#define COMMAND_ARG2WN(wn, idx) \
if(COMMAND_GETARG(wn, 64, idx)) { \
	cs_str wndot = String_LastChar(wn, '.'); \
	if(!wndot || !String_CaselessCompare(wndot, ".cws")) \
		String_Append(wn, 64, ".cws"); \
} else { \
	COMMAND_PRINTUSAGE; \
}

#define COMMAND_FUNC(N) \
static cs_bool svcmd_##N(CommandCallData *ccdata)

#define COMMAND_ADD(N, F) \
Command_Register(#N, (cmdFunc)svcmd_##N, F);

#define COMMAND_REMOVE(N) \
Command_UnregisterByFunc((cmdFunc)svcmd_##N);

enum {
	CMDF_NONE,
	CMDF_OP = BIT(0),
	CMDF_CLIENT = BIT(1),
	CMDF_RESERVED0 = BIT(2),
	CMDF_RESERVED1 = BIT(3),
	CMDF_RESERVED2 = BIT(4)
};

typedef struct {
	struct _Command *command;
	cs_str args;
	Client *caller;
	cs_char *out;
} CommandCallData;

typedef cs_bool(*cmdFunc)(CommandCallData *);

typedef struct _Command {
	cs_char alias[7];
	cmdFunc func;
	void *data;
	cs_byte flags;
} Command;

cs_bool Command_Handle(cs_char *cmd, Client *caller);

API Command *Command_Register(cs_str name, cmdFunc func, cs_byte flags);
API cs_bool Command_SetAlias(Command *cmd, cs_str alias);
API Command *Command_GetByName(cs_str name);
API void Command_Unregister(Command *cmd);
API void Command_UnregisterByFunc(cmdFunc func);
#endif // COMMAND_H
