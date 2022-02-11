#include "core.h"
#include "log.h"
#include "server.h"
#include "command.h"
#include "consoleio.h"
#include "strstor.h"
#ifdef CIO_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>

cs_bool rlalive = false,
rlupdated = false;

static cs_char *cmd_generator(cs_str text, cs_int32 state) {
	cs_size tlen = String_Length(text);
	if(tlen < 1) return NULL;
	AListField *field;

	List_Iter(field, Command_Head) {
		Command *cmd = AList_GetValue(field).ptr;
		if(String_CaselessCompare2(cmd->name, text, tlen)) {
			if(state-- > 0) continue;
			return (cs_char *)String_AllocCopy(cmd->name);
		}
	}

	return NULL;
}

static char *arg_generator(cs_str text, cs_int32 state) {
	cs_size tlen = String_Length(text);
	if(tlen < 1) return NULL;

	for(cs_int32 i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		cs_str name = Client_GetName(client);

		if(String_CaselessCompare2(name, text, tlen)) {
			if(state-- > 0) continue;
			return (cs_char *)String_AllocCopy(name);
		}
	}

	AListField *field;
	List_Iter(field, World_Head) {
		World *world = AList_GetValue(field).ptr;
		cs_str name = World_GetName(world);
		if(String_CaselessCompare2(name, text, tlen)) {
			if(state-- > 0) continue;
			return (cs_char *)String_AllocCopy(name);
		}
	}

	return NULL;
}

static char **cmd_completion(cs_str in, cs_int32 start, cs_int32 end) {
	(void)end;
	rl_attempted_completion_over = 1;
	if(start == 0)
		return rl_completion_matches(in, cmd_generator);
	else
		return rl_completion_matches(in, arg_generator);
	
	return NULL;
}
#endif

THREAD_FUNC(ConsoleIOThread) {
	(void)param;

	while(Server_Active) {
#ifdef CIO_USE_READLINE
		rlalive = true;
		cs_char *buf = readline(NULL);
		rlalive = false;
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
	if(rlalive) {
		rl_clear_visible_line();
		rl_redisplay();
		// TODO: Избавиться от этого непотребства
		File_Write("\x1B[2K\r", 5, 1, stderr);
		rlupdated = true;
	}
#endif
}

void ConsoleIO_AfterPrint(void) {
#ifdef CIO_USE_READLINE
	if(rlupdated) {
		rl_reset_line_state();
		rl_redisplay();
		rlupdated = false;
	}
#endif
}

cs_bool ConsoleIO_Init(void) {
#ifdef CIO_USE_READLINE
	rl_attempted_completion_function = cmd_completion;
#endif
	Thread_Create(ConsoleIOThread, NULL, true);
	return Console_BindSignalHandler(ConsoleIO_Handler);
}
