#include "core.h"
#include "log.h"
#include "server.h"
#include "command.h"
#include "consoleio.h"
#include "strstor.h"

static struct {
	void *lib;

	cs_char *(*start)(cs_str *prompt);
	cs_int32 (*clear)(void);
	cs_int32 (*reset)(void);
	void (*draw)(void);
	cs_char **(*match)(cs_str text, char *(*)(cs_str, cs_int32));
	void (*tohistory)(cs_str text);
	
	void **attemptfunc;
	int *complatt;
} ReadLine;

static cs_str rllib[] = {
#if defined(CORE_USE_WINDOWS)
	"libreadline8.dll",
	"readline8.dll",
#elif defined(CORE_USE_UNIX)
	"libreadline.so.8.1",
	"libreadline.so.8",
	"libreadline.so", // Опасненько
#endif
	NULL
};

static cs_str syms[] = {
	"readline", "rl_clear_visible_line",
	"rl_reset_line_state", "rl_redisplay",
	"rl_completion_matches", "add_history",
	"rl_attempted_completion_function",
	"rl_attempted_completion_over",

	NULL
};

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
	*ReadLine.complatt = 1;
	if(start == 0)
		return ReadLine.match(in, cmd_generator);
	else
		return ReadLine.match(in, arg_generator);
	
	return NULL;
}

void ConsoleIO_PrePrint(void) {
	if(ReadLine.lib && rlalive) {
		ReadLine.clear();
		ReadLine.draw();
		// TODO: Избавиться от этого непотребства
		File_Write("\x1B[2K\r", 5, 1, stderr);
		rlupdated = true;
	}
}

void ConsoleIO_AfterPrint(void) {
	if(ReadLine.lib && rlupdated) {
		rlupdated = false;
		ReadLine.reset();
		ReadLine.draw();
	}
}

static TSHND_RET sighand(TSHND_PARAM signal) {
	if(signal == CONSOLEIO_TERMINATE) Server_Active = false;
	return TSHND_OK;
}

THREAD_FUNC(ConsoleIOThread) {
	while(Server_Active) {
		if(ReadLine.lib) {
			rlalive = true;
			cs_char *buf = ReadLine.start(NULL);
			rlalive = false;
			if(buf && *buf != '\0') {
				ReadLine.tohistory(buf);
				if(Command_Handle(buf, NULL))
					continue;
			}
		} else {
			cs_char buf[192];
			if(File_ReadLine(stdin, buf, 192) > 0)
				if(Command_Handle(buf, NULL)) continue;
		}

		Log_Info(Sstor_Get("CMD_UNK"));
	}

	return 0;
}

cs_bool ConsoleIO_Init(void) {
	if(DLib_LoadAll(rllib, syms, (void **)&ReadLine))
		*ReadLine.attemptfunc = cmd_completion;

	Thread_Create(ConsoleIOThread, NULL, true);
	return Console_BindSignalHandler(sighand);
}
