#ifndef COMMAND_H
#define COMMAND_H
#include "client.h"

typedef bool (*cmdFunc)(char* args, CLIENT* caller, char* out);

typedef struct command {
	char*           name;
	cmdFunc         func;
	struct command* next;
} COMMAND;

COMMAND* headCommand;

void Command_Register(char* name, cmdFunc func);
bool Command_Handle(char* cmd, CLIENT* caller);
void Command_RegisterDefault();
#endif
