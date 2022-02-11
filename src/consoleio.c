#include "core.h"
#include "log.h"
#include "server.h"
#include "command.h"
#include "consoleio.h"
#include "strstor.h"
#ifdef CIO_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
cs_bool rlactive = false;
#endif

THREAD_FUNC(ConsoleIOThread) {
	(void)param;

	while(Server_Active) {
#ifdef CIO_USE_READLINE
		rlactive = true;
		cs_char *buf = readline("> ");
		rlactive = false;
		if(buf && *buf != '\0') {
			add_history(buf);
			if(Command_Handle(buf, NULL))
				continue;
		}
#else
		cs_char buf[192];
		if(File_ReadLine(stdin, buf, 192))
			if(Command_Handle(buf, NULL)) continue;
#endif
		Log_Info(Sstor_Get("CMD_UNK"));
	}

	return 0;
}

static TSHND_RET ConsoleIO_Handler(TSHND_PARAM signal) {
	if(signal == CONSOLEIO_TERMINATE) Server_Active = false;
	return TSHND_OK;
}

void ConsoleIO_PrePrint(void) {
#ifdef CIO_USE_READLINE
	if(!rlactive) return;
	rl_set_prompt("");
	rl_clear_visible_line();
	rl_redisplay();
	// TODO: Избавиться от этого непотребства
	File_Write("\x1B[2K\r", 5, 1, stderr);
#endif
}

void ConsoleIO_AfterPrint(void) {
#ifdef CIO_USE_READLINE
	if(!rlactive) return;
	rl_set_prompt("> ");
	rl_reset_line_state();
	rl_redisplay();
#endif
}

cs_bool ConsoleIO_Init(void) {
	Thread_Create(ConsoleIOThread, NULL, true);
	return Console_BindSignalHandler(ConsoleIO_Handler);
}
