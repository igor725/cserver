#include "core.h"
#include "platform.h"
#include "console.h"
#include "command.h"
#include "server.h"

THREAD conThread;

static int ReadLine(char* buf, int buflen) {
	int len = 0, c = 0;

	while((c = File_GetChar(stdin)) != EOF && c != '\n' && len < buflen)
		if(c != '\r') buf[len++] = (char)c;
	while(c != '\n' && File_GetChar(stdin) != '\n') {}
	buf[len] = '\0';

	return len;
}

static void HandleCommand(char* cmd) {
	if(!Command_Handle(cmd, NULL))
		Log_Info("Unknown command");
}

static TRET ConsoleThreadProc(TARG param) {
	char buf[CON_STR_LEN] = {0};
	(void)param;

	while(Server_Active) {
		if(ReadLine(buf, CON_STR_LEN) > 0 && Server_Active)
			HandleCommand(buf);
	}
	return 0;
}

void Console_StartListen(void) {
	conThread = Thread_Create(ConsoleThreadProc, NULL);
}

void Console_Close(void) {
	if(conThread) Thread_Close(conThread);
}
