#ifndef COMMAND_H
#define COMMAND_H
#include "str.h"
#include "client.h"
#include "lang.h"

#define Command_OnlyForClient \
if(!ccdata->caller) { \
	Lang_Get(LANG_CMDONLYCL); \
}

#define Command_OnlyForConsole \
if(ccdata->caller) { \
	Command_Print(Lang_Get(LANG_CMDONLYCON)); \
}

#define Command_Print(str) \
String_Copy(ccdata->out, MAX_CMD_OUT, str); \
return true;

#define Command_Printf(f, ...) \
String_FormatBuf(ccdata->out, MAX_CMD_OUT, f, __VA_ARGS__); \
return true;

#define Command_PrintUsage \
Command_Printf(Lang_Get(LANG_CMDUSAGE), cmdUsage);

#define Command_ArgToWorldName(wn, idx) \
if(String_GetArgument(ccdata->args, wn, 64, idx)) { \
	const char* wndot = String_LastChar(wn, '.'); \
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

#define Command_OnlyForOP \
if(ccdata->caller && !ccdata->caller->playerData->isOP) { \
	Command_Print(Lang_Get(LANG_CMDAD)); \
}

#define COMMAND_FUNC(N) \
static cs_bool svcmd_##N(CommandCallData* ccdata)

#define COMMAND_ADD(N) \
Command_Register(#N, svcmd_##N);

typedef struct {
	struct _Command* command;
	const char* args;
	Client* caller;
	char* out;
} CommandCallData;

typedef cs_bool(*cmdFunc)(CommandCallData* cdata);

typedef struct _Command {
	const char *name, *alias;
	cmdFunc func;
	void* data;
	struct _Command *next, *prev;
} Command;

API Command* Command_Register(const char* name, cmdFunc func);
API void Command_SetAlias(Command* cmd, const char* alias);
API Command* Command_Get(const char* name);
API cs_bool Command_Unregister(Command* cmd);
API cs_bool Command_UnregisterByName(const char* name);

void Command_RegisterDefault(void);
cs_bool Command_Handle(char* cmd, Client* caller);
#endif // COMMAND_H
