#include "core.h"
#include "log.h"
#include "console.h"
#include "command.h"

int Console_ReadLine(char* buf, int buflen) {
	int len = 0;
	int c = 0;

	while((c = getc(stdin)) != EOF && c != '\n') {
		if(c != '\r') {
			buf[len] = c;
			len++;
			if(len > buflen) {
				len--;
				break;
			}
		}
	}
	buf[len] = 0;

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
		if(Console_ReadLine(buf, 4096) > 0)
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

void Console_Close() {
	if(Console_Thread)
		CloseHandle(Console_Thread);
}
