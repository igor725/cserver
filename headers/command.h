#ifndef COMMAND_H
#define COMMAND_H
#include "client.h"
#include "lang.h"

#define	CMD_MAX_OUT 1024
#define Command_OnlyForClient \
if(!caller) { \
	String_Copy(out, CMD_MAX_OUT, Lang_Get(LANG_CMDONLYCL)); \
	return true; \
}

#define Command_OnlyForConsole \
if(caller) { \
	String_Copy(out, CMD_MAX_OUT, Lang_Get(LANG_CMDONLYCON)); \
	return true; \
}

#define Command_PrintUsage \
String_FormatBuf(out, CMD_MAX_OUT, Lang_Get(LANG_CMDUSAGE), cmdUsage); \
return true;

#define Command_Print(str) \
String_Copy(out, CMD_MAX_OUT, str); \
return true;

#define Command_ArgToWorldName(wn, idx) \
if(String_GetArgument(args, wn, 64, idx)) { \
	const char* wndot = String_LastChar(wn, '.'); \
	if(!wndot || !String_CaselessCompare(wndot, ".cws")) \
		String_Append(wn, 64, ".cws"); \
} else { \
	if(!caller) { \
		Command_PrintUsage; \
	} else { \
		PLAYERDATA pd = caller->playerData; \
		if(!pd) { \
			Command_PrintUsage; \
		} \
		WORLD world = pd->world; \
		if(!world) { \
			Command_PrintUsage; \
		} \
		String_Copy(wn, 64, world->name); \
	} \
}


#define Command_OnlyForOP \
if(caller && !caller->playerData->isOP) { \
	Command_Print(Lang_Get(LANG_CMDAD)); \
} \

typedef bool(*cmdFunc)(const char* args, CLIENT caller, char* out);

typedef struct command {
	const char* name;
	cmdFunc func;
	struct command* next;
	struct command* prev;
} *COMMAND;

API void Command_Register(const char* cmd, cmdFunc func);
API void Command_Unregister(const char* cmd);

void Command_RegisterDefault(void);
bool Command_Handle(char* cmd, CLIENT caller);
#endif
