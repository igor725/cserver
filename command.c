#include "core.h"
#include "log.h"
#include "command.h"
#include "packets.h"
#include "server.h"

void Command_Register(char* cmd, cmdFunc func) {
	COMMAND* tmp = calloc(1, sizeof(struct command));

	tmp->name = cmd;
	tmp->func = func;

	if(!tailCommand) {
		firstCommand = tmp;
		tailCommand = tmp;
	} else {
		tailCommand->next = tmp;
		tailCommand = tmp;
	}
}

bool CHandler_Stop(char* args, CLIENT* caller, char* out) {
	serverActive = false;
	return false;
}

bool CHandler_Test(char* args, CLIENT* caller, char* out) {
	sprintf(out, "Command \"test\" called by %s with args: %s",
		caller ? caller->playerData->name : "console", args
	);
	return true;
}

bool CHandler_Announce(char* args, CLIENT* caller, char* out) {
	Packet_WriteChat(caller, CPE_ANNOUNCE, "Test announcement");
	return false;
}

void Command_RegisterDefault() {
	Command_Register("stop", &CHandler_Stop);
	Command_Register("test", &CHandler_Test);
	Command_Register("announce", &CHandler_Announce);
}

bool Command_Handle(char* cmd, CLIENT* caller) {
	char* ret = calloc(512, 1);
	char* args = cmd;

	while(1) {
		++args;
		if(*args == 0) {
			args = NULL;
			break;
		} else if (*args == 32){
			*args = 0;
			++args;
			break;
		}
	}

	COMMAND* tmp = firstCommand;
	while(tmp) {
		if(String_CaselessCompare(tmp->name, cmd)) {
			if(tmp->func(args, caller, ret))
				if(caller)
					Packet_WriteChat(caller, 0, ret);
				else
					Log_Info(ret);
			free(ret);
			return true;
		}
		tmp = tmp->next;
	}

	free(ret);
	return false;
}
