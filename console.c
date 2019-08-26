#include "log.h"
#include "stdio.h"
#include "server.h"
#include "windows.h"
#include "console.h"
#include "command.h"

void* Console_Thread;

int Console_ReadLine(char* buf, int buflen) {
	int len = 0;
	int c = 0;

	while((c = getc(stdin)) != EOF && c != '\n') {
		if(c != '\r') {
			buf[len] = c;
			len++;
		}
	}

	return len;
}

void Console_HandleCommand(char* cmd) {
	if(!Command_Handle(cmd, NULL)) {
		Log_Info("Unknown command");
	}
}

int Console_ThreadProc(void* lpParam) {
	char buf[4096] = {0};

	while(1) {
		int len = Console_ReadLine(buf, 4096);

		if(len > 0)
			Console_HandleCommand(buf);
	}
	return 0;
}

void Console_StartListen() {
	Console_Thread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)&Console_ThreadProc,
		NULL,
		0,
		NULL
	);
}

void Console_StopListen() {
	if(Console_Thread)
		CloseHandle(Console_Thread);
}
