#ifndef COMMAND_H
#define COMMAND_H
#include "client.h"

#define	CMD_MAX_OUT 512

typedef bool (*cmdFunc)(const char* args, CLIENT* caller, char* out);

typedef struct command {
	const char*     name;
	cmdFunc         func;
	bool            onlyOP;
	struct command* next;
} COMMAND;

COMMAND* headCommand;

void Command_Register(const char* cmd, cmdFunc func, bool onlyOP);
bool Command_Handle(char* cmd, CLIENT* caller);
void Command_RegisterDefault();
#endif
