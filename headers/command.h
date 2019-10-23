#ifndef COMMAND_H
#define COMMAND_H
#include "client.h"

#define	CMD_MAX_OUT 1024
#define Command_OnlyForClient \
if(!caller) { \
	String_Copy(out, CMD_MAX_OUT, "This command can't be used from console."); \
	return true; \
}

#define Command_OnlyForConsole \
if(caller) { \
	String_Copy(out, CMD_MAX_OUT, "This command can be used only from console."); \
	return true; \
}

#define Command_PrintUsage \
String_Copy(out, CMD_MAX_OUT, "Usage: "); \
String_Append(out, CMD_MAX_OUT, cmdUsage); \
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
	Command_PrintUsage; \
}


#define Command_OnlyForOP \
if(caller && !caller->playerData->isOP) { \
	String_Copy(out, CMD_MAX_OUT, "Access denied"); \
	return true; \
} \

typedef bool (*cmdFunc)(const char* args, CLIENT caller, char* out);

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
