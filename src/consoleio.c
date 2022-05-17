#include "core.h"
#include "log.h"
#include "server.h"
#include "command.h"
#include "consoleio.h"
#include "strstor.h"
#include "client.h"
#include "world.h"
#include "list.h"
#include "platform.h"

static struct {
	void *lib;

	cs_char *(*start)(cs_str *prompt);
	cs_int32 (*clear)(void);
	cs_int32 (*reset)(void);
	void (*draw)(void);
	cs_char **(*match)(cs_str text, char *(*)(cs_str, cs_int32));
	void (*tohistory)(cs_str text);

	void **attemptfunc;
	cs_int32 *complatt;

	void (*prep)(cs_int32 meta);
	void (*deprep)(void);
} ReadLine;

static cs_str const syms[] = {
	"readline", "rl_clear_visible_line",
	"rl_reset_line_state", "rl_redisplay",
	"rl_completion_matches", "add_history",

	"rl_attempted_completion_function",
	"rl_attempted_completion_over",

	"rl_prep_terminal", "rl_deprep_terminal",

	NULL
};

/*
 * Windows не поддерживает libreadline.
 * Имеется порт 5ой, но и его не получится
 * заставить работать. Так как нужна как
 * минимум 7ая версия ридлайна.
 * 
 * В libreadline.dylib.8 по какой-то
 * мистической причине отсутствует символ
 * rl_clear_visible_line, соответственно
 * библиотека не будет использоваться.
 * Нужно либо придумать способ обойтись
 * без этой функции, либо же забить.
 * Профиты от первого варианта:
 * 	- ConsoleIO начнёт рабоать под MacOS
 * 	- Станет возможной поддержка версий
 * 	библиотеки libreadline ниже 8.
*/

static cs_str const rllib[] = {
#ifdef CORE_USE_UNIX
	"libreadline." DLIB_EXT ".8",
	"libreadline." DLIB_EXT ".7",
	// Ниже 7ой версии отсутствуют некоторые API
	"libreadline." DLIB_EXT "", // Опасненько
#endif
	NULL
};

Thread inputThread = (Thread)NULL;
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

static cs_char *arg_generator(cs_str text, cs_int32 state) {
	cs_size tlen = String_Length(text);
	if(tlen < 1) return NULL;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
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
	(void)param;
	while(Server_Active) {
		if(ReadLine.lib) {
			rlalive = true;
			cs_char *buf = ReadLine.start(NULL);
			rlalive = false;
			if(buf) {
				if(*buf != '\0') {
					ReadLine.tohistory(buf);
					if(Command_Handle(buf, NULL)) {
						Memory_Free(buf);
						continue;
					}
				}
				Memory_Free(buf);
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
		*ReadLine.attemptfunc = (void *)cmd_completion;

	inputThread = Thread_Create(ConsoleIOThread, NULL, false);
	return Console_BindSignalHandler(sighand);
}

void ConsoleIO_Uninit(void) {
	if(ReadLine.lib)
		ReadLine.deprep();
}
