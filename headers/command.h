#ifndef COMMAND_H
#define COMMAND_H
#include "client.h"

#define	CMD_MAX_OUT 512
#define Command_OnlyForClient \
if(!caller) { \
	String_Copy(out, CMD_MAX_OUT, "This command can't be used from console."); \
	return true; \
} \

typedef bool (*cmdFunc)(const char* args, CLIENT* caller, char* out);

typedef struct command {
	const char*     name;
	cmdFunc         func;
	bool            onlyOP;
	struct command* next;
} COMMAND;

COMMAND* headCommand;

API void Command_Register(const char* cmd, cmdFunc func, bool onlyOP);
bool Command_Handle(char* cmd, CLIENT* caller);
void Command_RegisterDefault();
#endif
