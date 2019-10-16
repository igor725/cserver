#include "core.h"
#include "console.h"
#include "command.h"
#include "server.h"

THREAD conThread;

int Console_ReadLine(char* buf, int buflen) {
	int len = 0, c = 0;

	while((c = getc(stdin)) != EOF && c != '\n') {
		if(c != '\r') {
			buf[len++] = (char)c;
			if(len > buflen) {
				break;
			}
		}
	}

	buf[len] = 0;
	return len;
}

void Console_HandleCommand(char* cmd) {
	if(*cmd == '/') ++cmd;
	if(!Command_Handle(cmd, NULL))
		Log_Info("Unknown command");
}

TRET Console_ThreadProc(TARG lpParam) {
	char buf[CON_STR_LEN] = {0};
	(void)lpParam;

	while(Server_Active) {
		if(Console_ReadLine(buf, CON_STR_LEN) > 0 && Server_Active)
			Console_HandleCommand(buf);
	}
	return 0;
}

void Console_StartListen(void) {
	conThread = Thread_Create(Console_ThreadProc, NULL);
}

void Console_Close(void) {
	if(conThread) Thread_Close(conThread);
}
