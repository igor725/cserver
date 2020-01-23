#ifndef COMMAND_H
#define COMMAND_H
#include "str.h"
#include "client.h"
#include "lang.h"

#define Command_SetUsage(str) \
static cs_str cmdUsage = str;

#define Command_Print(str) \
String_Copy(ccdata->out, MAX_CMD_OUT, str); \
return true;

#define Command_Printf(f, ...) \
String_FormatBuf(ccdata->out, MAX_CMD_OUT, f, __VA_ARGS__); \
return true;

#define Command_Append(str) \
String_Append(ccdata->out, MAX_CMD_OUT, str);

#define Command_Appendf(buf, sz, fmt, ...) \
String_FormatBuf(buf, sz, fmt, __VA_ARGS__); \
String_Append(ccdata->out, MAX_CMD_OUT, buf);

#define Command_GetArg(a, s, n) \
String_GetArgument(ccdata->args, a, s, n)

#define Command_PrintUsage \
Command_Printf(Lang_Get(LANG_CMDUSAGE), cmdUsage);

// TODO: Сделать это добро функцией
#define Command_ArgToWorldName(wn, idx) \
if(Command_GetArg(wn, 64, idx)) { \
	cs_str wndot = String_LastChar(wn, '.'); \
	if(!wndot || !String_CaselessCompare(wndot, ".cws")) \
		String_Append(wn, 64, ".cws"); \
} else { \
	if(!ccdata->caller) { \
		Command_PrintUsage; \
	} else { \
		PlayerData* pd = ccdata->caller->playerData; \
		if(!pd) { \
			Command_PrintUsage; \
		} \
		World* world = pd->world; \
		if(!world) { \
			Command_PrintUsage; \
		} \
		String_Copy(wn, 64, world->name); \
	} \
}

#define COMMAND_FUNC(N) \
static cs_bool svcmd_##N(CommandCallData* ccdata)

#define COMMAND_ADD(N, F) \
Command_Register(#N, (cmdFunc)svcmd_##N, F);

#define COMMAND_REMOVE(N) \
Command_UnregisterByFunc((cmdFunc)svcmd_##N);

enum {
	CMDF_NONE,
	CMDF_OP = (1 << 1),
	CMDF_CLIENT = (1 << 2),
	CMDF_RESERVED0 = (1 << 3),
	CMDF_RESERVED1 = (1 << 4),
	CMDF_RESERVED2 = (1 << 5)
};

typedef struct {
	struct _Command* command;
	cs_str args;
	Client* caller;
	char* out;
} CommandCallData;

typedef cs_bool(*cmdFunc)(CommandCallData* cdata);

typedef struct _Command {
	const char* alias;
	cs_uint8 flags;
	cmdFunc func;
	void* data;
} Command;

API Command* Command_Register(cs_str name, cmdFunc func, cs_uint8 flags);
API void Command_SetAlias(Command* cmd, cs_str alias);
API Command* Command_GetByName(cs_str name);
API void Command_Unregister(Command* cmd);
API void Command_UnregisterByFunc(cmdFunc func);

void Command_RegisterDefault(void);
cs_bool Command_Handle(char* cmd, Client* caller);
#endif // COMMAND_H
