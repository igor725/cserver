#ifndef COMMAND_H
#define COMMAND_H
#include "str.h"
#include "client.h"
#include "lang.h"

#define Command_OnlyForClient(a) \
if(!(a)->caller) { \
	Lang_Get(LANG_CMDONLYCL); \
}

#define Command_OnlyForConsole(a) \
if((a)->caller) { \
	Command_Print((a), Lang_Get(LANG_CMDONLYCON)); \
}

#define Command_Print(a, str) \
String_Copy((a)->out, MAX_CMD_OUT, str); \
return true;

#define Command_Printf(a, f, ...) \
String_FormatBuf((a)->out, MAX_CMD_OUT, f, __VA_ARGS__); \
return true;

#define Command_PrintUsage(a) \
Command_Printf(a, Lang_Get(LANG_CMDUSAGE), cmdUsage);

#define Command_ArgToWorldName(a, wn, idx) \
if(String_GetArgument((a)->args, wn, 64, idx)) { \
	const char* wndot = String_LastChar(wn, '.'); \
	if(!wndot || !String_CaselessCompare(wndot, ".cws")) \
		String_Append(wn, 64, ".cws"); \
} else { \
	if(!(a)->caller) { \
		Command_PrintUsage((a)); \
	} else { \
		PlayerData pd = (a)->caller->playerData; \
		if(!pd) { \
			Command_PrintUsage((a)); \
		} \
		World world = pd->world; \
		if(!world) { \
			Command_PrintUsage((a)); \
		} \
		String_Copy(wn, 64, world->name); \
	} \
}

#define Command_OnlyForOP(a) \
if((a)->caller && !(a)->caller->playerData->isOP) { \
	Command_Print((a), Lang_Get(LANG_CMDAD)); \
}

typedef struct _CommandCallData {
	struct _Command* command;
	const char* args;
	Client caller;
	char* out;
} *CommandCallData;

typedef cs_bool(*cmdFunc)(CommandCallData cdata);

typedef struct _Command {
	const char *name, *alias;
	cmdFunc func;
	void* data;
	struct _Command *next, *prev;
} *Command;

API Command Command_Register(const char* name, cmdFunc func);
API void Command_SetAlias(Command cmd, const char* alias);
API Command Command_Get(const char* name);
API cs_bool Command_Unregister(Command cmd);
API cs_bool Command_UnregisterByName(const char* name);

void Command_RegisterDefault(void);
cs_bool Command_Handle(char* cmd, Client caller);
#endif // COMMAND_H
